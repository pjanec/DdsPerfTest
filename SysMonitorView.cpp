#include "stdafx.h"
#include "SysMonitorView.h"
#include "App.h"
#include "imgui.h"
#include <iomanip>
#include <sstream> // Required for std::stringstream
#include <chrono>  // Required for time point manipulation
#include <ctime>   // Required for std::tm

namespace DdsPerfTest
{

SysMonitorView::SysMonitorView(App* app) : _app(app)
{
    _participant = _app->GetParticipant(0);

    _topic = dds_create_topic(_participant, &Net_SystemMonitorSample_desc, "SystemMonitorSample", NULL, NULL);
    if (_topic < 0) DDS_FATAL("dds_create_topic failed for SystemMonitorSample: %s", dds_strretcode(-_topic));

    _reader = dds_create_reader(_participant, _topic, NULL, NULL);
    if (_reader < 0) DDS_FATAL("dds_create_reader failed for SystemMonitorSample: %s", dds_strretcode(-_reader));
}

SysMonitorView::~SysMonitorView()
{
    if (_reader >= 0) dds_delete(_reader);
    if (_topic >= 0) dds_delete(_topic);


    CloseCsvFile();
}

void SysMonitorView::Tick()
{
    ReadSamples();
}

void SysMonitorView::ReadSamples()
{
    const int MAX_SAMPLES = 50; // Read up to 50 samples per tick
    void* samples[MAX_SAMPLES] = {0};
    dds_sample_info_t infos[MAX_SAMPLES];
    int n = dds_take(_reader, samples, infos, MAX_SAMPLES, MAX_SAMPLES);
    if (n < 0) {
        // This can happen if the entity is being deleted. Avoid fatal error during shutdown.
        printf("dds_take failed in SysMonitorView: %s\n", dds_strretcode(-n));
        return;
    }

    for (int i = 0; i < n; i++)
    {
        if (infos[i].valid_data)
        {
            auto* sample = static_cast<Net_SystemMonitorSample*>(samples[i]);
            auto& history = _historySamples[sample->computerName];
            history.push_back(*sample);
            
            // Prune old samples to prevent memory leak
            auto now = std::chrono::system_clock::now();
            while (!history.empty()) {
                auto sampleTime = std::chrono::time_point<std::chrono::system_clock>() + std::chrono::nanoseconds(history.front().timestampUtc);
                auto age = std::chrono::duration_cast<std::chrono::seconds>(now - sampleTime).count();
                if (age > _maxHistorySeconds) {
                    history.pop_front();
                } else {
                    break;
                }
            }
            
            if (_recordToCsv)
            {
                WriteToCsv(*sample);
            }
        }
    }

    dds_return_loan(_reader, samples, n);

}

void SysMonitorView::DrawUI()
{
    DrawChartWindows(); // Draw any open chart windows
    
    ImGui::Begin("System Performance Monitor");

    auto sysMonitor = _app->GetSysMonitor();
    if (sysMonitor && sysMonitor->IsMonitorInstance())
    {
        ImGui::PushItemWidth(200);

        float& intervalSec = _app->GetMonitorIntervalSec();

        if (ImGui::SliderFloat("Sample Interval", &intervalSec, 0.1f, 10.0f, "%.2f s", ImGuiSliderFlags_Logarithmic))
        {
            // Clamp value to be safe
            if (intervalSec < 0.1f) intervalSec = 0.1f;
            if (intervalSec > 10.0f) intervalSec = 10.0f;

            sysMonitor->SetSampleInterval((int)(intervalSec * 1000.0f));
        }

        ImGui::PopItemWidth();

        ImGui::SameLine();
    }

    if (ImGui::Checkbox("Record Performance to CSV", &_recordToCsv))
    {
        SetRecordingEnabled(_recordToCsv);
    }

    ImGui::Separator();

    if (ImGui::BeginTable("PerfStatsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Computer Name");
        ImGui::TableSetupColumn("CPU (%)");
        ImGui::TableSetupColumn("Mem Used (MB)");
        ImGui::TableSetupColumn("Network (Mbps)");
        ImGui::TableHeadersRow();

        for (const auto& [name, history] : _historySamples)
        {
            if (history.empty()) continue;
            
            const auto& sample = history.back();
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0); 
            ImGui::Text("%s", name.c_str());
            
            // CPU Column - clickable
            ImGui::TableSetColumnIndex(1);
            std::stringstream cpuStr;
            cpuStr << std::fixed << std::setprecision(2) << sample.cpuUsagePercent;
            if (ImGui::Selectable(cpuStr.str().c_str(), false))
                OpenChartWindow(name, PerfMetric::CPU);
            
            // Memory Column - clickable
            ImGui::TableSetColumnIndex(2);
            std::stringstream memStr;
            memStr << std::fixed << std::setprecision(0) << sample.memoryUsageMb;
            if (ImGui::Selectable(memStr.str().c_str(), false))
                OpenChartWindow(name, PerfMetric::Memory);
            
            // Network Column - clickable
            ImGui::TableSetColumnIndex(3);
            std::stringstream netStr;
            netStr << std::fixed << std::setprecision(3) << sample.networkUsageMbps;
            if (ImGui::Selectable(netStr.str().c_str(), false))
                OpenChartWindow(name, PerfMetric::Network);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void SysMonitorView::SetRecordingEnabled(bool enabled)
{
    _recordToCsv = enabled;
    _recordToCsv ? OpenCsvFile() : CloseCsvFile();
}

void SysMonitorView::OpenCsvFile()
{
    if (_csvFile.is_open()) return;
    std::string filename = "performance_log_" + _app->GetAppId().ComputerName + ".csv";
    _csvFile.open(filename, std::ios_base::app);
    _csvFile.seekp(0, std::ios::end);
    if (_csvFile.tellp() == 0)
    {
        _csvFile << "TimestampUTC,ComputerName,CPUUsagePercent,MemoryUsedMB,NetworkUsageMbps\n";
    }
}

void SysMonitorView::CloseCsvFile()
{
    if (_csvFile.is_open()) _csvFile.close();
}

std::string SysMonitorView::FormatTimestampISO(long long timestampUtc)
{
    // Create a time_point from the nanoseconds duration since the system_clock epoch.
    auto tp = std::chrono::time_point<std::chrono::system_clock>() + std::chrono::nanoseconds(timestampUtc);

    // Get the whole seconds part as time_t
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::seconds>(tp));

    // Calculate the millisecond part from the original time_point
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count() % 1000;

    // Convert the UTC time_t to a local time tm struct (use localtime_s for thread safety on Windows)
    std::tm t_local;
    localtime_s(&t_local, &tt);

    // Use a stringstream to format into ISO 8601 format with timezone offset
    std::stringstream ss;
    ss << std::put_time(&t_local, "%Y-%m-%dT%H:%M:%S") 
       << "." << std::setw(3) << std::setfill('0') << millis
       << std::put_time(&t_local, "%z"); // Append the timezone offset (e.g., +0200)
       
    // The %z format gives "+0200", but standard ISO 8601 prefers a colon "+02:00".
    // We can manually insert it.
    std::string result = ss.str();
    result.insert(result.length() - 2, ":");
       
    return result;
}

void SysMonitorView::WriteToCsv(const Net_SystemMonitorSample& sample)
{
    if (!_csvFile.is_open()) return;

    // --- CALL THE NEW HELPER METHOD ---
    std::string timestampStr = FormatTimestampISO(sample.timestampUtc);

    _csvFile << timestampStr << "," // Use the formatted string
        << sample.computerName << ","
        << std::fixed << std::setprecision(2) << sample.cpuUsagePercent << ","
        << std::fixed << std::setprecision(0) << sample.memoryUsageMb << ","
        << std::fixed << std::setprecision(3) << sample.networkUsageMbps << "\n";
}

void SysMonitorView::OpenChartWindow(const std::string& computerName, PerfMetric metric)
{
    // Check if a window for this metric already exists
    for (auto& win : _chartWindows)
    {
        if (win.ComputerName == computerName && win.Metric == metric)
        {
            win.IsOpen = true;
            ImGui::SetWindowFocus(win.WindowId.c_str());
            return;
        }
    }

    // If not, create a new one
    ChartWindowState newState;
    newState.IsOpen = true;
    newState.ComputerName = computerName;
    newState.Metric = metric;
    
    const char* metricName = "";
    switch (metric)
    {
        case PerfMetric::CPU: metricName = "CPU"; break;
        case PerfMetric::Memory: metricName = "Memory"; break;
        case PerfMetric::Network: metricName = "Network"; break;
    }
    
    std::stringstream windowId;
    windowId << metricName << " - " << computerName;
    newState.WindowId = windowId.str();
    
    _chartWindows.push_back(newState);
}

void SysMonitorView::DrawChartWindows()
{
    for (auto& chart : _chartWindows)
    {
        if (!chart.IsOpen) continue;

        ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(chart.WindowId.c_str(), &chart.IsOpen))
        {
            // --- Time Window and Scaling Controls ---
            ImGui::PushItemWidth(80);
            ImGui::InputFloat("History (s)", &chart.TimeWindowSec, 1.0f, 5.0f, "%.1f");
            if (chart.TimeWindowSec < 1.f) chart.TimeWindowSec = 1.f;
            if (chart.TimeWindowSec > _maxHistorySeconds) chart.TimeWindowSec = (float)_maxHistorySeconds;
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::Checkbox("Auto-scale", &chart.AutoScale);

            ImGui::SameLine();
            // Disable the input box if auto-scaling is on
            if (chart.AutoScale)
            {
                ImGui::BeginDisabled();
            }
            ImGui::PushItemWidth(80);
            ImGui::InputFloat("Max Y", &chart.ManualMax);
            ImGui::PopItemWidth();
            if (chart.AutoScale)
            {
                ImGui::EndDisabled();
            }


            // --- Prepare data for plotting ---
            std::vector<float> plotData;
            const auto& history = _historySamples[chart.ComputerName];
            auto now = std::chrono::system_clock::now();
            float minVal = 0.f, maxVal = chart.ManualMax;
            std::string latestValueStr = "N/A";

            if (chart.AutoScale)
            {
                minVal = FLT_MAX;
                maxVal = FLT_MIN;
            }

            for (const auto& sample : history)
            {
                auto sampleTime = std::chrono::time_point<std::chrono::system_clock>() + std::chrono::nanoseconds(sample.timestampUtc);
                auto age = std::chrono::duration_cast<std::chrono::duration<float>>(now - sampleTime).count();

                if (age <= chart.TimeWindowSec)
                {
                    float value = 0.0f;
                    switch (chart.Metric)
                    {
                    case PerfMetric::CPU:     value = sample.cpuUsagePercent; break;
                    case PerfMetric::Memory:  value = sample.memoryUsageMb;   break;
                    case PerfMetric::Network: value = sample.networkUsageMbps; break;
                    }
                    plotData.push_back(value);
                    if (chart.AutoScale)
                    {
                        if (value < minVal) minVal = value;
                        if (value > maxVal) maxVal = value;
                    }
                }
            }

            if (!plotData.empty())
            {
                switch (chart.Metric)
                {
                case PerfMetric::CPU:     latestValueStr = std::format("{:.2f} %%", plotData.back()); break;
                case PerfMetric::Memory:  latestValueStr = std::format("{:.0f} MB Used", plotData.back()); break;
                case PerfMetric::Network: latestValueStr = std::format("{:.3f} Mbps", plotData.back()); break;
                }
            }

            // Adjust plot range for better visualization when auto-scaling
            if (chart.AutoScale)
            {
                if (maxVal == minVal) {
                    maxVal += 1.0f;
                }
                float range = maxVal - minVal;
                maxVal += range * 0.1f; // 10% padding on top
                minVal = minVal > 0 ? minVal - range * 0.1f : 0; // 10% padding on bottom, but not below 0
                if (minVal < 0.f) minVal = 0.f;
            }

            // --- Draw Axis Labels and Plot ---
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 plotCursorPos = ImGui::GetCursorScreenPos();

            const float plotHeight = 150.0f;
            const float labelPadding = 5.0f; // Padding from the plot edge
            float chartLeft = plotCursorPos.x + 50; // Leave space for labels
            float chartWidth = ImGui::GetContentRegionAvail().x - 50;

            // Draw Y-axis labels
            drawList->AddText(ImVec2(plotCursorPos.x, plotCursorPos.y), ImGui::GetColorU32(ImGuiCol_Text), std::format("{:.1f}", maxVal).c_str());
            drawList->AddText(ImVec2(plotCursorPos.x, plotCursorPos.y + plotHeight - ImGui::GetTextLineHeight()), ImGui::GetColorU32(ImGuiCol_Text), std::format("{:.1f}", minVal).c_str());

            // Manually position the plot to the right of the labels
            ImGui::SetCursorScreenPos(ImVec2(chartLeft, plotCursorPos.y));
            ImGui::PlotLines(latestValueStr.c_str(), plotData.data(), plotData.size(), 0, nullptr, minVal, maxVal, ImVec2(chartWidth, plotHeight));
        }
        ImGui::End();
    }
}
} // namespace DdsPerfTest