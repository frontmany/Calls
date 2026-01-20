#include "crashCatchInitializer.h"
#include "CrashCatch.hpp"

#include <filesystem>

namespace core::utilities
{
    bool initializeCrashCatch(const std::string& dumpFilePath, const std::string& appVersion)
    {
        static bool initialized = false;
        if (initialized) {
            return true;
        }

        try {
            std::filesystem::path dumpPath = dumpFilePath.empty()
                ? std::filesystem::path("calliforniaCore")
                : std::filesystem::path(dumpFilePath);

            std::filesystem::path dumpDir = dumpPath.parent_path();
            std::string dumpFileName = dumpPath.filename().string();

            if (dumpDir.empty()) {
                dumpDir = std::filesystem::current_path();
            }
            else {
                std::filesystem::create_directories(dumpDir);
            }

            std::string dumpFolderStr = dumpDir.string();
            if (!dumpFolderStr.empty() && dumpFolderStr.back() != '/' && dumpFolderStr.back() != '\\') {
                dumpFolderStr += "/";
            }

            CrashCatch::Config config;
            config.appVersion = appVersion;
            config.dumpFolder = dumpFolderStr;
            config.dumpFileName = dumpFileName.empty() ? "calliforniaCore" : dumpFileName;

            initialized = CrashCatch::initialize(config);
            return initialized;
        }
        catch (...) {
            return false;
        }
    }
}
