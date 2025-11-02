#include <iostream>
#include <filesystem>

#include "callsServer.h"
#include "logger.h"

int main()
{
    std::filesystem::create_directories("logs");
    
    LOG_INFO("=== Calls Server Starting ===");
    LOG_INFO("Server port: 8081");
    
    try {
        CallsServer server("8081");
        server.run();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}
