#include "stdafx.h"
#include "SysMonitor.h"
#include "App.h"
#include "tinyxml2/tinyxml2.h"

#include <winsock2.h>   // must be included before ws2tcpip.h / windows.h
#include <ws2tcpip.h>   // getnameinfo, InetPton/InetNtop, NI_* flags, INET6_ADDRSTRLEN
#include <iphlpapi.h>   // GetAdaptersAddresses, IP_ADAPTER_ADDRESSES
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")


namespace DdsPerfTest
{

SysMonitor::SysMonitor(App* app) : _app(app)
{
    _singletonMutex = CreateMutex(NULL, TRUE, L"Global\\DdsPerfTest_MonitorMutex_9A3B8C2D");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        _isMonitorInstance = false;
        CloseHandle(_singletonMutex);
        _singletonMutex = NULL;
        return;
    }
    _isMonitorInstance = true;
    Initialize();
}

SysMonitor::~SysMonitor()
{
    if (_isMonitorInstance)
    {
        _stopThread = true;
        if (_monitorThread.joinable()) { _monitorThread.join(); }

        if (_writer >= 0) dds_delete(_writer);
        if (_topic >= 0) dds_delete(_topic);

        ClosePdh();
        if (_singletonMutex)
        {
            ReleaseMutex(_singletonMutex);
            CloseHandle(_singletonMutex);
        }
    }
}

void SysMonitor::Initialize()
{
    if (!_isMonitorInstance) return;
    _participant = _app->GetParticipant(0);

    _topic = dds_create_topic(_participant, &Net_SystemMonitorSample_desc, "SystemMonitorSample", NULL, NULL);
    if (_topic < 0) DDS_FATAL("dds_create_topic failed for SystemMonitorSample: %s", dds_strretcode(-_topic));

    _writer = dds_create_writer(_participant, _topic, NULL, NULL);
    if (_writer < 0) DDS_FATAL("dds_create_writer failed for SystemMonitorSample: %s", dds_strretcode(-_writer));

    InitializePdh();

    _monitorThread = std::thread(&SysMonitor::MonitorThreadFunc, this);
}

void SysMonitor::SetSampleInterval(int intervalMs)
{
    _sampleIntervalMs = (intervalMs < 100) ? 100 : intervalMs;
}

void SysMonitor::MonitorThreadFunc()
{
    Sleep(1000); // Initial sleep for PDH counters
    while (!_stopThread)
    {
        float cpu = 0.0f, mem = 0.0f, net = 0.0f;
        if (CollectPerformanceData(cpu, mem, net))
        {
            PublishPerformanceData(cpu, mem, net);
        }
        Sleep(_sampleIntervalMs);
    }
}

void SysMonitor::InitializePdh()
{
    if (PdhOpenQuery(NULL, 0, &_pdhQuery) != ERROR_SUCCESS) return;

    _networkInterfaceName = FindNetworkInterfaceToMonitor();

    if (!_networkInterfaceName.empty())
    {
        printf("SysMonitor: Monitoring network performance on interface: '%s'\n", _networkInterfaceName.c_str());
    }
    else
    {
        printf("SysMonitor: WARNING - Could not determine a network interface to monitor. Network stats will be unavailable.\n");
    }

    PdhAddCounter(_pdhQuery, L"\\Processor Information(_Total)\\% Processor Time", 0, &_cpuCounter);
    PdhAddCounter(_pdhQuery, L"\\Memory\\Available MBytes", 0, &_memCounter);

    if (!_networkInterfaceName.empty())
    {
        std::wstring pdhPath = L"\\Network Interface(" + std::wstring(_networkInterfaceName.begin(), _networkInterfaceName.end()) + L")\\Bytes Total/sec";
        PdhAddCounter(_pdhQuery, pdhPath.c_str(), 0, &_netCounter);
    }
    
    PdhCollectQueryData(_pdhQuery);
}

bool SysMonitor::CollectPerformanceData(float& cpu, float& mem, float& net)
{
    if (PdhCollectQueryData(_pdhQuery) != ERROR_SUCCESS) return false;
    PDH_FMT_COUNTERVALUE val;

    if (PdhGetFormattedCounterValue(_cpuCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) cpu = (float)val.doubleValue;
    if (PdhGetFormattedCounterValue(_memCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) mem = (float)val.doubleValue;
    if (_netCounter && PdhGetFormattedCounterValue(_netCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS)
    {
        net = (float)(val.doubleValue * 8.0 / (1024.0 * 1024.0)); // Bytes/sec to Mbps
    }
    return true;
}

void SysMonitor::PublishPerformanceData(float cpu, float mem, float net)
{
    Net_SystemMonitorSample sample = { 0 };
    auto now = std::chrono::system_clock::now();
    sample.timestampUtc = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    sample.computerName = (char*)_app->GetAppId().ComputerName.c_str();
    sample.cpuUsagePercent = cpu;
    sample.memoryUsageMb = mem;
    sample.networkUsageMbps = net;
    dds_write(_writer, &sample);
}

void SysMonitor::ClosePdh()
{
    if (_pdhQuery) { PdhCloseQuery(_pdhQuery); _pdhQuery = NULL; }
}

std::string SysMonitor::FindNetworkInterfaceToMonitor()
{
    // Priority 1: Try to get from CycloneDDS config
    std::string interfaceIp = GetInterfaceFromCycloneConfig();
    if (!interfaceIp.empty())
    {
        std::string interfaceName = GetInterfaceNameFromIp(interfaceIp);
        if (!interfaceName.empty())
        {
            // The name from GetInterfaceNameFromIp might need special character replacement for PDH
            // e.g. '/' becomes '_'
            for (char& c : interfaceName) {
                if (c == '/' || c == '(' || c == ')' || c == '#') {
                    c = '_';
                }
            }
            return interfaceName;
        }
    }

    // Priority 2: Fallback to busiest interface
    return FindBusiestNetworkInterface();
}

std::string SysMonitor::GetInterfaceFromCycloneConfig()
{
    // get the CYCLONEDDS_URI environment variable
    char buf[1024] = { 0 };
    size_t requiredSize;
    errno_t err = getenv_s(&requiredSize, buf, sizeof(buf) - 1, "CYCLONEDDS_URI");
    if (err != 0 || requiredSize == 0) {
        //std::cerr << "CYCLONEDDS_URI not set." << std::endl;
        return "";
    }

    // skip the file:// part
    const char* uri = buf;
    if (strncmp(uri, "file://", 7) == 0) {
        uri += 7;
    }

    // Load the XML file
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(uri) != tinyxml2::XML_SUCCESS) {
        //std::cerr << "Error loading XML file. Path: " << uri << std::endl;
        return "";
    }

    // Get the root element
    tinyxml2::XMLElement* root = doc.FirstChildElement("CycloneDDS");
    if (!root) {
        //std::cerr << "No CycloneDDS element found in XML." << std::endl;
        return "";
    }

    tinyxml2::XMLElement* domain = root->FirstChildElement("Domain");
    if (!domain) {
        //std::cerr << "No Domain element found in XML." << std::endl;
        return "";
    }

    tinyxml2::XMLElement* general = domain->FirstChildElement("General");
    if (!general) {
        //std::cerr << "No General element found in XML." << std::endl;
        return "";
    }

    tinyxml2::XMLElement* interfaces = general->FirstChildElement("Interfaces");
    if (!interfaces) {
        //std::cerr << "No Interfaces element found in XML." << std::endl;
        return "";
    }

    tinyxml2::XMLElement* networkInterface = interfaces->FirstChildElement("NetworkInterface");
    if (!networkInterface) {
        //std::cerr << "No NetworkInterface element found in XML." << std::endl;
        return "";
    }

    // Access and print the content of the located element
    const char* address = networkInterface->Attribute("address");
    if (!address) {
        //std::cout << "Address of 'NetworkInterface': " << address << std::endl;
        return "";
    }

    return address;
}


std::string SysMonitor::FindBusiestNetworkInterface()
{
    PDH_HQUERY tempQuery;
    if (PdhOpenQuery(NULL, 0, &tempQuery) != ERROR_SUCCESS) return "";

    DWORD bufferSize = 0, itemCount = 0;
    PdhEnumObjectItems(NULL, NULL, L"Network Interface", NULL, &bufferSize, NULL, &itemCount, PERF_DETAIL_WIZARD, 0);
    if (bufferSize == 0) {
        PdhCloseQuery(tempQuery);
        return "";
    }
    
    std::vector<wchar_t> instanceNames(bufferSize);
    if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", instanceNames.data(), &bufferSize, NULL, &itemCount, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS) {
        PdhCloseQuery(tempQuery);
        return "";
    }

    std::vector<PDH_HCOUNTER> counters;
    std::vector<std::wstring> names;
    for (const wchar_t* instance = instanceNames.data(); *instance; instance += wcslen(instance) + 1) {
        std::wstring name(instance);
        if (name.find(L"Loopback") != std::wstring::npos || name.find(L"isatap") != std::wstring::npos) continue;
        std::wstring path = L"\\Network Interface(" + name + L")\\Bytes Total/sec";
        PDH_HCOUNTER counter;
        if (PdhAddCounter(tempQuery, path.c_str(), 0, &counter) == ERROR_SUCCESS) {
            counters.push_back(counter);
            names.push_back(name);
        }
    }

    if (counters.empty()) { PdhCloseQuery(tempQuery); return ""; }

    PdhCollectQueryData(tempQuery);
    Sleep(1000);
    PdhCollectQueryData(tempQuery);

    double maxBytes = -1.0;
    std::wstring busiestName = L"";
    for (size_t i = 0; i < counters.size(); ++i) {
        PDH_FMT_COUNTERVALUE value;
        if (PdhGetFormattedCounterValue(counters[i], PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS && value.CStatus == ERROR_SUCCESS && value.doubleValue > maxBytes) {
            maxBytes = value.doubleValue;
            busiestName = names[i];
        }
    }

    PdhCloseQuery(tempQuery);
    if (!busiestName.empty()) {
        printf("Determined busiest network interface: %S\n", busiestName.c_str());
        return std::string(busiestName.begin(), busiestName.end());
    }
    return "";
}

    std::string SysMonitor::GetInterfaceNameFromIp(const std::string& ipAddress)
    {
        // This function iterates through all network adapters on the system to find
        // the one that matches the provided IP address. It returns the "Description"
        // of the adapter, which is the name PDH uses for performance counters.

        ULONG bufferSize = 0;
        // Call once to get the required buffer size for the adapter information.
        GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &bufferSize);

        if (bufferSize == 0) return "";

        std::vector<BYTE> buffer(bufferSize);
        IP_ADAPTER_ADDRESSES* pAddresses = (IP_ADAPTER_ADDRESSES*)buffer.data();

        // Call a second time to get the actual adapter information.
        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize) == NO_ERROR)
        {
            // Iterate through the linked list of adapters.
            for (IP_ADAPTER_ADDRESSES* pAdapter = pAddresses; pAdapter != NULL; pAdapter = pAdapter->Next)
            {
                // For each adapter, iterate through its assigned unicast IP addresses (both IPv4 and IPv6).
                for (IP_ADAPTER_UNICAST_ADDRESS* pUnicast = pAdapter->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next)
                {
                    char addrStr[INET6_ADDRSTRLEN];
                    // Convert the binary socket address to a numeric string.
                    getnameinfo(pUnicast->Address.lpSockaddr, pUnicast->Address.iSockaddrLength, addrStr, sizeof(addrStr), NULL, 0, NI_NUMERICHOST);

                    // If the string IP address matches our target...
                    if (ipAddress == addrStr)
                    {
                        // ...return the adapter's Description, converted from WCHAR* to std::string.
                        std::wstring wideDescription(pAdapter->Description);
                        return std::string(wideDescription.begin(), wideDescription.end());
                    }
                }
            }
        }

        // Return an empty string if no matching adapter was found.
        return "";
    }
} // namespace DdsPerfTest