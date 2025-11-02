cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(clientUpdaterPy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

pybind11_add_module(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE client_updater)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../clientUpdater/src
    ../vendor/json/include                               
    ../vendor/asio/asio/include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:RelWithDebInfo>:DEBUG>  
)