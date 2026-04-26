#pragma once

#include <chrono>
#include <cstdint>
#include <utility>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

namespace server::utilities
{
    struct ProcessMetrics {
        int32_t pid = 0;
        std::string name;
        double cpuUsagePercent = 0.0;
        uint64_t memoryRssBytes = 0;
        uint32_t threads = 0;
        uint64_t fdCount = 0;
        uint64_t uptimeSec = 0;
    };

    struct SystemMetricsSnapshot {
        double cpuUsagePercent = 0.0;
        uint64_t memoryUsedBytes = 0;
        uint64_t memoryAvailableBytes = 0;
        std::vector<ProcessMetrics> processes;
    };

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

    inline SystemMetricsSnapshot collectSystemMetricsSnapshot(size_t maxProcesses = 15, std::chrono::milliseconds sampleWindow = std::chrono::milliseconds(200)) {
        (void)maxProcesses;
        (void)sampleWindow;
        auto [memoryUsed, memoryAvailable] = getMemoryUsedAndAvailable();
        SystemMetricsSnapshot snapshot;
        snapshot.cpuUsagePercent = getCpuUsagePercent();
        snapshot.memoryUsedBytes = memoryUsed;
        snapshot.memoryAvailableBytes = memoryAvailable;
        return snapshot;
    }
#elif defined(__linux__)
    namespace
    {
        struct LinuxCpuTimes {
            uint64_t total = 0;
            uint64_t idleTotal = 0;
            bool valid = false;
        };

        struct LinuxProcessSample {
            int32_t pid = 0;
            std::string name;
            uint64_t totalCpuJiffies = 0;
            uint64_t processStartJiffies = 0;
            uint64_t memoryRssBytes = 0;
            uint32_t threads = 0;
            uint64_t fdCount = 0;
        };

        inline LinuxCpuTimes readLinuxCpuTimes() {
            std::ifstream stat("/proc/stat");
            if (!stat) {
                return {};
            }

            std::string line;
            if (!std::getline(stat, line) || line.compare(0, 4, "cpu ") != 0) {
                return {};
            }

            uint64_t user = 0;
            uint64_t nice = 0;
            uint64_t system = 0;
            uint64_t idle = 0;
            uint64_t iowait = 0;
            uint64_t irq = 0;
            uint64_t softirq = 0;
            uint64_t steal = 0;
            std::istringstream iss(line.substr(4));
            iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            LinuxCpuTimes times;
            times.total = user + nice + system + idle + iowait + irq + softirq + steal;
            times.idleTotal = idle + iowait;
            times.valid = times.total > 0;
            return times;
        }

        inline uint64_t readLinuxSystemUptimeSec() {
            std::ifstream uptime("/proc/uptime");
            if (!uptime) {
                return 0;
            }

            double uptimeSeconds = 0.0;
            uptime >> uptimeSeconds;
            if (uptimeSeconds < 0.0) {
                return 0;
            }
            return static_cast<uint64_t>(uptimeSeconds);
        }

        inline uint64_t countProcessFds(int32_t pid) {
            const std::filesystem::path fdPath = std::filesystem::path("/proc") / std::to_string(pid) / "fd";
            std::error_code ec;
            if (!std::filesystem::exists(fdPath, ec) || ec) {
                return 0;
            }

            uint64_t count = 0;
            std::filesystem::directory_iterator it(fdPath, std::filesystem::directory_options::skip_permission_denied, ec);
            if (ec) {
                return 0;
            }
            for (const auto& entry : it) {
                (void)entry;
                ++count;
            }
            return count;
        }

        inline bool readLinuxProcessSample(int32_t pid, LinuxProcessSample& sample) {
            const std::filesystem::path statPath = std::filesystem::path("/proc") / std::to_string(pid) / "stat";
            std::ifstream statFile(statPath);
            if (!statFile) {
                return false;
            }

            std::string line;
            if (!std::getline(statFile, line)) {
                return false;
            }

            const size_t openParen = line.find('(');
            const size_t closeParen = line.rfind(')');
            if (openParen == std::string::npos || closeParen == std::string::npos || closeParen <= openParen) {
                return false;
            }

            sample.pid = pid;
            sample.name = line.substr(openParen + 1, closeParen - openParen - 1);

            const size_t restOffset = closeParen + 2;
            if (restOffset >= line.size()) {
                return false;
            }

            std::istringstream rest(line.substr(restOffset));
            std::vector<std::string> fields;
            std::string token;
            while (rest >> token) {
                fields.push_back(token);
            }

            // fields index starts from stat field #3 (state).
            if (fields.size() <= 21) {
                return false;
            }

            uint64_t utime = 0;
            uint64_t stime = 0;
            uint64_t numThreads = 0;
            uint64_t startTime = 0;
            try {
                utime = std::stoull(fields[11]);      // field #14
                stime = std::stoull(fields[12]);      // field #15
                numThreads = std::stoull(fields[17]); // field #20
                startTime = std::stoull(fields[19]);  // field #22
            } catch (...) {
                return false;
            }

            const std::filesystem::path statmPath = std::filesystem::path("/proc") / std::to_string(pid) / "statm";
            std::ifstream statmFile(statmPath);
            uint64_t rssPages = 0;
            if (statmFile) {
                uint64_t totalPages = 0;
                statmFile >> totalPages >> rssPages;
            }
            const long pageSize = sysconf(_SC_PAGESIZE);
            sample.memoryRssBytes = (pageSize > 0) ? rssPages * static_cast<uint64_t>(pageSize) : 0;

            sample.totalCpuJiffies = utime + stime;
            sample.processStartJiffies = startTime;
            sample.threads = static_cast<uint32_t>(numThreads);
            sample.fdCount = countProcessFds(pid);
            return true;
        }

        inline std::unordered_map<int32_t, LinuxProcessSample> readAllLinuxProcessSamples() {
            std::unordered_map<int32_t, LinuxProcessSample> samples;
            const std::filesystem::path procPath("/proc");
            std::error_code ec;
            std::filesystem::directory_iterator it(procPath, std::filesystem::directory_options::skip_permission_denied, ec);
            if (ec) {
                return samples;
            }

            for (const auto& entry : it) {
                if (!entry.is_directory(ec) || ec) {
                    continue;
                }

                const std::string name = entry.path().filename().string();
                if (name.empty() || !std::all_of(name.begin(), name.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) {
                    continue;
                }

                int32_t pid = 0;
                try {
                    pid = std::stoi(name);
                } catch (...) {
                    continue;
                }

                LinuxProcessSample sample;
                if (readLinuxProcessSample(pid, sample)) {
                    samples.emplace(pid, std::move(sample));
                }
            }
            return samples;
        }
    }

    inline double getCpuUsagePercent() {
        const LinuxCpuTimes first = readLinuxCpuTimes();
        if (!first.valid) {
            return 0.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        const LinuxCpuTimes second = readLinuxCpuTimes();
        if (!second.valid || second.total <= first.total || second.idleTotal < first.idleTotal) {
            return 0.0;
        }

        const uint64_t diffTotal = second.total - first.total;
        const uint64_t diffIdle = second.idleTotal - first.idleTotal;
        if (diffTotal == 0) {
            return 0.0;
        }
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

    inline SystemMetricsSnapshot collectSystemMetricsSnapshot(size_t maxProcesses = 15, std::chrono::milliseconds sampleWindow = std::chrono::milliseconds(200)) {
        SystemMetricsSnapshot snapshot;
        auto [memoryUsed, memoryAvailable] = getMemoryUsedAndAvailable();
        snapshot.memoryUsedBytes = memoryUsed;
        snapshot.memoryAvailableBytes = memoryAvailable;

        const LinuxCpuTimes cpuBefore = readLinuxCpuTimes();
        auto processesBefore = readAllLinuxProcessSamples();

        std::this_thread::sleep_for(sampleWindow);

        const LinuxCpuTimes cpuAfter = readLinuxCpuTimes();
        auto processesAfter = readAllLinuxProcessSamples();

        if (!cpuBefore.valid || !cpuAfter.valid || cpuAfter.total <= cpuBefore.total || cpuAfter.idleTotal < cpuBefore.idleTotal) {
            return snapshot;
        }

        const uint64_t diffTotal = cpuAfter.total - cpuBefore.total;
        const uint64_t diffIdle = cpuAfter.idleTotal - cpuBefore.idleTotal;
        if (diffTotal == 0) {
            return snapshot;
        }
        snapshot.cpuUsagePercent = 100.0 * (1.0 - static_cast<double>(diffIdle) / static_cast<double>(diffTotal));

        const long clockTicks = sysconf(_SC_CLK_TCK);
        const uint64_t systemUptimeSec = readLinuxSystemUptimeSec();

        snapshot.processes.reserve(processesAfter.size());
        for (const auto& [pid, processAfter] : processesAfter) {
            auto beforeIt = processesBefore.find(pid);
            if (beforeIt == processesBefore.end()) {
                continue;
            }

            const uint64_t cpuBeforeJiffies = beforeIt->second.totalCpuJiffies;
            const uint64_t cpuAfterJiffies = processAfter.totalCpuJiffies;
            if (cpuAfterJiffies < cpuBeforeJiffies) {
                continue;
            }

            const uint64_t diffProcess = cpuAfterJiffies - cpuBeforeJiffies;
            const double processCpuUsage = (diffTotal == 0)
                ? 0.0
                : (100.0 * static_cast<double>(diffProcess) / static_cast<double>(diffTotal));

            uint64_t processUptimeSec = 0;
            if (clockTicks > 0) {
                const uint64_t processStartSec = processAfter.processStartJiffies / static_cast<uint64_t>(clockTicks);
                processUptimeSec = systemUptimeSec > processStartSec ? (systemUptimeSec - processStartSec) : 0;
            }

            ProcessMetrics processMetrics;
            processMetrics.pid = processAfter.pid;
            processMetrics.name = processAfter.name;
            processMetrics.cpuUsagePercent = processCpuUsage;
            processMetrics.memoryRssBytes = processAfter.memoryRssBytes;
            processMetrics.threads = processAfter.threads;
            processMetrics.fdCount = processAfter.fdCount;
            processMetrics.uptimeSec = processUptimeSec;
            snapshot.processes.push_back(std::move(processMetrics));
        }

        std::sort(snapshot.processes.begin(), snapshot.processes.end(), [](const ProcessMetrics& lhs, const ProcessMetrics& rhs) {
            if (lhs.cpuUsagePercent != rhs.cpuUsagePercent) {
                return lhs.cpuUsagePercent > rhs.cpuUsagePercent;
            }
            if (lhs.memoryRssBytes != rhs.memoryRssBytes) {
                return lhs.memoryRssBytes > rhs.memoryRssBytes;
            }
            return lhs.pid < rhs.pid;
        });

        if (snapshot.processes.size() > maxProcesses) {
            snapshot.processes.resize(maxProcesses);
        }

        return snapshot;
    }
#else
    inline double getCpuUsagePercent() { return 0.0; }
    inline std::pair<uint64_t, uint64_t> getMemoryUsedAndAvailable() { return {0, 0}; }
    inline SystemMetricsSnapshot collectSystemMetricsSnapshot(size_t maxProcesses = 15, std::chrono::milliseconds sampleWindow = std::chrono::milliseconds(200)) {
        (void)maxProcesses;
        (void)sampleWindow;
        return {};
    }
#endif
}
