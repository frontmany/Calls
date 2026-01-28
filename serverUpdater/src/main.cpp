#include <filesystem>
#include <thread>
#include <chrono>

#include "serverUpdater.h"
#include "utilities/logger.h"

namespace serverUpdater
{
	const std::filesystem::path versionsDirectory = "versions";
}

int main()
{
    std::filesystem::create_directories("logs");
    
    LOG_INFO("=== Server Updater Starting ===");
    
    try {
        serverUpdater::ServerUpdater server(8082, serverUpdater::versionsDirectory);
        server.start();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}

