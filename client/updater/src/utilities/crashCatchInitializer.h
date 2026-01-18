#pragma once

#include <string>

namespace updater::utilities
{
    bool initializeCrashCatch(const std::string& dumpFilePath, const std::string& appVersion);
}
