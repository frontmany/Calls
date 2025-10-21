cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(callsServerPy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

file(GLOB SOURCES "src/*.cpp" "src/*.h")

# Add server source files (excluding main.cpp)
file(GLOB SERVER_SOURCES 
    "../server/src/*.cpp"
    "../server/src/*.h"
)
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

pybind11_add_module(callsServerPy ${SOURCES} ${SERVER_SOURCES})

if(WIN32)
    set(CRYPTOPP_DIR "${CMAKE_SOURCE_DIR}/vendor/cryptopp/x64/Output")
else()
    set(CRYPTOPP_DIR "../vendor/cryptopp")
endif()

target_link_directories(callsServerPy PRIVATE
    "${CRYPTOPP_DIR}"
)

if(WIN32)
    target_link_libraries(callsServerPy PRIVATE
        "${CRYPTOPP_DIR}/$<CONFIG>/cryptlib.lib"
    )
else()
    target_link_libraries(callsServerPy PRIVATE
        libcryptopp.a
    )
endif()

target_include_directories(callsServerPy PRIVATE
    ../server/src
    ../vendor/opus/include     
    ../vendor/portaudio/include
    ../vendor/json/include                    
    ../vendor/cryptopp               
    ../vendor/asio/asio/include
)