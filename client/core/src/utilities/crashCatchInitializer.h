#pragma once

#include <string>

namespace core::utilities
{
    bool initializeCrashCatch(const std::string& dumpFilePath, const std::string& appVersion);
}
