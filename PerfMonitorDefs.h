#pragma once

#include <string>
#include <cstdint>

namespace DdsPerfTest
{
    // Defines the performance metric being displayed in a chart.
    enum class PerfMetric
    {
        CPU,
        Memory,
        NetworkSent,
        NetworkReceived
    };

    // Holds the state for an individual performance chart window.
    struct ChartWindowState
    {
        bool IsOpen = false;
        std::string ComputerName;
        PerfMetric Metric;
        float TimeWindowSec = 10.0f;
        std::string WindowId;
        bool AutoScale = true;
        float ManualMax = 100.0f;
    };

    // A lightweight struct for storing performance data in the history deque,
    // using stable pointers from a string cache.
    struct PerformanceSample
    {
        const char* computerName;
        const char* ipAddress;
        int64_t timestampUtc;
        float cpuUsagePercent;
        float memoryUsageMb;
        float networkSentMbps;
        float networkReceivedMbps;
    };
}
