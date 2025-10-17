#pragma once

#include "NetworkDefs.h"
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <deque>
#include <unordered_set>
#include <string_view>

// Include the new definition files
#include "PerfMonitorDefs.h"
#include "StringCache.h"

namespace DdsPerfTest { class App; }

namespace DdsPerfTest
{
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
        void WriteToCsv(const PerformanceSample& sample);
        void OpenCsvFile();
        void CloseCsvFile();
        std::string FormatTimestampISO(long long timestampUtc);
        void OpenChartWindow(const std::string& computerName, PerfMetric metric);
        void DrawChartWindows();

        App* _app;
        int _participant = -1;
        int _topic = -1;
        int _reader = -1;
          
        // The history deque now stores our new lightweight struct.
        std::map<std::string, std::deque<PerformanceSample>> _historySamples;
        const size_t _maxHistorySeconds = 60;
          
        // Update the cache declaration to use the transparent hasher.
        std::unordered_set<std::string, StringViewHasher, std::equal_to<>> _stringCache;
          
        std::vector<ChartWindowState> _chartWindows;
        bool _recordToCsv = false;
        std::ofstream _csvFile;
    };
}