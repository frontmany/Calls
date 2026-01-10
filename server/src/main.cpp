#include <iostream>
#include <filesystem>

#include "server.h"
#include "utilities/logger.h"

int main()
{
    std::filesystem::create_directories("logs");
    
    
    try {
        server::Server server("8081");
        server.run();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}
