#include "updaterDiagnostics.h"
#include "logger.h"
#include "crashCatchInitializer.h"

#include <filesystem>

namespace updater::utilities
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

        updater::utilities::log::set_log_directory(logDir.string());

        std::filesystem::path crashDir = crashDumpDirectory.empty()
            ? basePath / "crashDumps"
            : std::filesystem::path(crashDumpDirectory);
        if (crashDir.is_relative()) {
            crashDir = basePath / crashDir;
        }

        initializeCrashCatch((crashDir / "callifornia_updater").string(),
            appVersion);
    }
}
