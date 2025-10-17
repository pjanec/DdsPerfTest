#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <Pdh.h> // For performance counters

// Forward declare to avoid including App.h here
namespace DdsPerfTest { class App; }

namespace DdsPerfTest
{
    class SysMonitor
    {
    public:
        SysMonitor(App* app);
        ~SysMonitor();

        void SetSampleInterval(int intervalMs);
        bool IsMonitorInstance() const { return _isMonitorInstance; }

    private:
        void Initialize();
        void MonitorThreadFunc();

        void InitializePdh();
        bool CollectPerformanceData(float& cpu, float& mem, float& net);
        void ClosePdh();

        std::string FindNetworkInterfaceToMonitor();
        std::string GetInterfaceFromCycloneConfig();
        std::string FindBusiestNetworkInterface();
        std::string GetInterfaceNameFromIp(const std::string& ipAddress);

        void PublishPerformanceData(float cpu, float mem, float net);
        
        App* _app;
        HANDLE _singletonMutex = NULL;
        bool _isMonitorInstance = false;

        std::thread _monitorThread;
        std::atomic<bool> _stopThread = false;
        std::atomic<int> _sampleIntervalMs = 1000;

        int _participant = -1;
        int _topic = -1;
        int _writer = -1;

        PDH_HQUERY _pdhQuery = NULL;
        PDH_HCOUNTER _cpuCounter = NULL;
        PDH_HCOUNTER _memCounter = NULL;
        PDH_HCOUNTER _netCounter = NULL;
        std::string _networkInterfaceName;
        float _totalMemoryMb = 0.0f;
    };
}