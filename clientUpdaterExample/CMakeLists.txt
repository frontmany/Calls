cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(clientUpdaterExample)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} client_updater)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ../clientUpdater/src
    ../vendor/json/include                                  
    ../vendor/asio/asio/include
    )