cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(clientUpdaterPy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

pybind11_add_module(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build/Release")
else()
    set(SPDLOG_DIR "../vendor/spdlog")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "${SPDLOG_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        spdlog.lib
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libspdlog.a
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE client_updater)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../clientUpdater/src
    ../vendor/spdlog/include
    ../vendor/json/include                               
    ../vendor/asio/asio/include
    ../vendor/spdlog/include
)