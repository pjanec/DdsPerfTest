#pragma once

#include "NetworkDefs.h"
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <deque>

namespace DdsPerfTest { class App; }

namespace DdsPerfTest
{
    enum class PerfMetric
    {
        CPU,
        Memory,
        NetworkSent,
        NetworkReceived
    };

    struct ChartWindowState
    {
        bool IsOpen = false;
        std::string ComputerName;
        PerfMetric Metric;
        float TimeWindowSec = 10.0f;
        std::string WindowId; // Unique ID for ImGui
        bool AutoScale = true;
        float ManualMax = 100.0f;
    };

    class SysMonitorView
    {
    public:
        SysMonitorView(App* app);
        ~SysMonitorView();

        void Tick();
        void DrawUI();

        bool IsRecordingEnabled() const { return _recordToCsv; }
        void SetRecordingEnabled(bool enabled);

    private:
        void ReadSamples();
        void WriteToCsv(const Net_SystemMonitorSample& sample);
        void OpenCsvFile();
        void CloseCsvFile();
        std::string FormatTimestampISO(long long timestampUtc);
        void OpenChartWindow(const std::string& computerName, PerfMetric metric);
        void DrawChartWindows();

        App* _app;
        int _participant = -1;
        int _topic = -1;
        int _reader = -1;
        
        // Replace _latestSamples with history:
        std::map<std::string, std::deque<Net_SystemMonitorSample>> _historySamples;
        const size_t _maxHistorySeconds = 60; // Store up to a minute of data
        
        // Add this to manage chart windows:
        std::vector<ChartWindowState> _chartWindows;
        
        bool _recordToCsv = false;
        std::ofstream _csvFile;
    };
}