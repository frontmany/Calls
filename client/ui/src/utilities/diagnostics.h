#pragma once

#include <string>

namespace ui::utilities
{
    void initializeDiagnostics(const std::string& appDirectory,
        const std::string& logDirectory,
        const std::string& crashDumpDirectory,
        const std::string& appVersion);
}
 