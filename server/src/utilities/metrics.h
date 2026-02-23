#pragma once

#include <chrono>
#include <cstdint>
#include <utility>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <fstream>
#include <sstream>
#endif

namespace server::utilities
{
#ifdef _WIN32
    namespace
    {
        inline uint64_t fileTimeToUint64(const FILETIME& ft) {
            return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        }
    }

    inline double getCpuUsagePercent() {
        FILETIME idle1, kernel1, user1, idle2, kernel2, user2;
        if (!GetSystemTimes(&idle1, &kernel1, &user1))
            return 0.0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!GetSystemTimes(&idle2, &kernel2, &user2))
            return 0.0;

        uint64_t uIdle1 = fileTimeToUint64(idle1);
        uint64_t uKernel1 = fileTimeToUint64(kernel1);
        uint64_t uUser1 = fileTimeToUint64(user1);
        uint64_t uIdle2 = fileTimeToUint64(idle2);
        uint64_t uKernel2 = fileTimeToUint64(kernel2);
        uint64_t uUser2 = fileTimeToUint64(user2);

        uint64_t diffIdle = uIdle2 - uIdle1;
        uint64_t diffKernel = uKernel2 - uKernel1;
        uint64_t diffUser = uUser2 - uUser1;
        uint64_t diffTotal = diffKernel + diffUser;

        if (diffTotal == 0)
            return 0.0;
        return 100.0 * (1.0 - static_cast<double>(diffIdle) / static_cast<double>(diffTotal));
    }

    inline std::pair<uint64_t, uint64_t> getMemoryUsedAndAvailable() {
        MEMORYSTATUSEX memInfo = {};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (!GlobalMemoryStatusEx(&memInfo))
            return {0, 0};
        uint64_t totalPhys = memInfo.ullTotalPhys;
        uint64_t availPhys = memInfo.ullAvailPhys;
        uint64_t usedPhys = totalPhys - availPhys;
        return {usedPhys, availPhys};
    }
#elif defined(__linux__)
    inline double getCpuUsagePercent() {
        std::ifstream stat1("/proc/stat");
        if (!stat1) return 0.0;
        std::string line;
        if (!std::getline(stat1, line) || line.compare(0, 4, "cpu ") != 0)
            return 0.0;
        stat1.close();

        uint64_t user1 = 0, nice1 = 0, system1 = 0, idle1 = 0, iowait1 = 0, irq1 = 0, softirq1 = 0, steal1 = 0;
        std::istringstream iss1(line.substr(4));
        iss1 >> user1 >> nice1 >> system1 >> idle1 >> iowait1 >> irq1 >> softirq1 >> steal1;
        uint64_t total1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1 + steal1;
        uint64_t idleTotal1 = idle1 + iowait1;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::ifstream stat2("/proc/stat");
        if (!stat2) return 0.0;
        if (!std::getline(stat2, line) || line.compare(0, 4, "cpu ") != 0)
            return 0.0;

        uint64_t user2 = 0, nice2 = 0, system2 = 0, idle2 = 0, iowait2 = 0, irq2 = 0, softirq2 = 0, steal2 = 0;
        std::istringstream iss2(line.substr(4));
        iss2 >> user2 >> nice2 >> system2 >> idle2 >> iowait2 >> irq2 >> softirq2 >> steal2;
        uint64_t total2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2 + steal2;
        uint64_t idleTotal2 = idle2 + iowait2;

        uint64_t diffTotal = total2 - total1;
        uint64_t diffIdle = idleTotal2 - idleTotal1;
        if (diffTotal == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(diffIdle) / static_cast<double>(diffTotal));
    }

    inline std::pair<uint64_t, uint64_t> getMemoryUsedAndAvailable() {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo) return {0, 0};

        uint64_t memTotalKb = 0, memAvailableKb = 0, memFreeKb = 0;
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.compare(0, 9, "MemTotal:") == 0)
                memTotalKb = std::stoull(line.substr(9));
            else if (line.compare(0, 14, "MemAvailable:") == 0)
                memAvailableKb = std::stoull(line.substr(14));
            else if (line.compare(0, 8, "MemFree:") == 0)
                memFreeKb = std::stoull(line.substr(8));
        }
        uint64_t availKb = memAvailableKb > 0 ? memAvailableKb : memFreeKb;
        if (memTotalKb == 0) return {0, 0};
        uint64_t totalBytes = memTotalKb * 1024;
        uint64_t availBytes = availKb * 1024;
        uint64_t usedBytes = totalBytes > availBytes ? totalBytes - availBytes : 0;
        return {usedBytes, availBytes};
    }
#else
    inline double getCpuUsagePercent() { return 0.0; }
    inline std::pair<uint64_t, uint64_t> getMemoryUsedAndAvailable() { return {0, 0}; }
#endif
}
