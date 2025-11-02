cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(serverUpdaterPy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

file(GLOB SERVER_SOURCES 
    "../serverUpdater/src/*.cpp"
    "../serverUpdater/src/*.h"
)
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

pybind11_add_module(${PROJECT_NAME} ${SOURCES} ${SERVER_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ../serverUpdater/src
    ../vendor/json/include                                   
    ../vendor/asio/asio/include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:RelWithDebInfo>:DEBUG>  
)