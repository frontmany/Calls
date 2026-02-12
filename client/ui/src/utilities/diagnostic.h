#pragma once

#include <string>
#include "logger.h"
#include "CrashCatch.hpp"

namespace ui::utilities
{
    void initializeDiagnostics(const std::string& appDirectory,
        const std::string& logDirectory,
        const std::string& crashDumpDirectory,
        const std::string& appVersion)
    {
        std::filesystem::path basePath = appDirectory.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(appDirectory);
        basePath = std::filesystem::absolute(basePath);

        std::filesystem::path logDir = logDirectory.empty()
            ? basePath / "logs"
            : std::filesystem::path(logDirectory);

        if (logDir.is_relative()) {
            logDir = basePath / logDir;
        }

        set_log_directory(logDir.string());

        std::filesystem::path dumpDir = crashDumpDirectory.empty()
            ? basePath / "crashDumps"
            : std::filesystem::path(crashDumpDirectory);

        if (dumpDir.is_relative()) {
            dumpDir = basePath / dumpDir;
        }

        std::filesystem::create_directories(dumpDir);

        std::string dumpFolderStr = dumpDir.string();
        if (!dumpFolderStr.empty() && dumpFolderStr.back() != '/' && dumpFolderStr.back() != '\\') {
            dumpFolderStr += "/";
        }

        CrashCatch::Config crashConfig;
        crashConfig.appVersion = appVersion;
        crashConfig.dumpFolder = dumpFolderStr;
        crashConfig.dumpFileName = "calliforniaUI";
        crashConfig.onCrash = [](const CrashCatch::CrashContext& context)
            {
                std::cout << "Crash captured. Dump: " << context.dumpFilePath
                    << " Log: " << context.logFilePath << std::endl;
            };

        CrashCatch::initialize(crashConfig);
    }
}
 