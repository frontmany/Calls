cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(callsClientPy VERSION 0.1.0 LANGUAGES CXX)

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

pybind11_add_module(callsClientPy ${SOURCES})

if(WIN32)
    target_compile_definitions(callsClientPy PRIVATE _WIN32_WINNT=0x0601)
endif()

if(WIN32)
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
else()
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "${SPDLOG_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libspdlog.a
    )
endif()

target_link_libraries(callsClientPy PRIVATE calliforniaCore)

target_include_directories(callsClientPy PRIVATE
    ${ROOT_DIR}/client/core/src
    ${ROOT_DIR}/vendor/opus/include     
    ${ROOT_DIR}/vendor/portaudio/include
    ${ROOT_DIR}/vendor/json/include                    
    ${ROOT_DIR}/vendor/cryptopp               
    ${ROOT_DIR}/vendor/asio/asio/include
    ${ROOT_DIR}/vendor/spdlog/include
    ${ROOT_DIR}/vendor/ticTimer
)