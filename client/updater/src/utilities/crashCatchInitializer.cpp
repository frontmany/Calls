#include "CrashCatch.hpp"

namespace
{
    bool initializeCrashCatch()
    {
        CrashCatch::Config config;
        config.dumpFileName = "callifornia_updater";

        return CrashCatch::initialize(config);
    }

    const bool crashCatchInitialized = initializeCrashCatch();
}
