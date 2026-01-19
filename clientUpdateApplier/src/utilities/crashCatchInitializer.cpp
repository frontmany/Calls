#include "crashCatchInitializer.h"
#include "CrashCatch.hpp"

#include <filesystem>

namespace updateApplier::utilities
{
    bool initializeCrashCatch(const std::string& dumpFilePath, const std::string& appVersion)
    {
        static bool initialized = false;
        if (initialized) {
            return true;
        }

        try {
            std::filesystem::path dumpPath = dumpFilePath.empty()
                ? std::filesystem::path("calliforniaUpdateApplier")
                : std::filesystem::path(dumpFilePath);

            if (!dumpPath.parent_path().empty()) {
                std::filesystem::create_directories(dumpPath.parent_path());
            }

            CrashCatch::Config config;
            config.appVersion = appVersion;
            config.dumpFileName = dumpPath.string();

            initialized = CrashCatch::initialize(config);
            return initialized;
        }
        catch (...) {
            return false;
        }
    }
}
