#pragma once

#include <string>

namespace updateApplier::utilities
{
    bool initializeCrashCatch(const std::string& dumpFilePath, const std::string& appVersion);
}
