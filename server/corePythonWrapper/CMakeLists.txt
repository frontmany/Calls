cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(callsServerPy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

# Add server source files (excluding main.cpp)
file(GLOB_RECURSE SERVER_SOURCES 
    "${ROOT_DIR}/server/core/src/*.cpp"
    "${ROOT_DIR}/server/core/src/*.h"
)
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

pybind11_add_module(callsServerPy ${SOURCES} ${SERVER_SOURCES})

if(WIN32)
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp/x64/Output")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
else()
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
endif()

target_link_directories(callsServerPy PRIVATE
    "${CRYPTOPP_DIR}"
    "${SPDLOG_DIR}"
)

if(WIN32)
    target_link_libraries(callsServerPy PRIVATE
        "$<IF:$<CONFIG:Debug>,${CRYPTOPP_DIR}/Debug/cryptlib.lib,${CRYPTOPP_DIR}/Release/cryptlib.lib>"
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
else()
    target_link_libraries(callsServerPy PRIVATE
        libcryptopp.a
        libspdlog.a
    )
endif()

target_include_directories(callsServerPy PRIVATE
    ${ROOT_DIR}/server/core/src
    ${ROOT_DIR}/vendor/opus/include     
    ${ROOT_DIR}/vendor/portaudio/include
    ${ROOT_DIR}/vendor/json/include                    
    ${ROOT_DIR}/vendor/cryptopp               
    ${ROOT_DIR}/vendor/asio/asio/include
    ${ROOT_DIR}/vendor/spdlog/include
)