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

set(PORTAUDIO_DIR "${ROOT_DIR}/vendor/portaudio/build")

if (WIN32) 
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp/x64/Output")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
else()
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
endif()

set(FFMPEG_DIR "${ROOT_DIR}/vendor/ffmpeg")
target_link_directories(${PROJECT_NAME} PRIVATE
    "${PORTAUDIO_DIR}"
    "${CRYPTOPP_DIR}"
    "${SPDLOG_DIR}"
    "${FFMPEG_DIR}/lib"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${PORTAUDIO_DIR}/$<CONFIG>/portaudio_static_x64.lib"
        "${CRYPTOPP_DIR}/$<CONFIG>/cryptlib.lib"
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
        "${FFMPEG_DIR}/lib/avcodec.lib"
        "${FFMPEG_DIR}/lib/avformat.lib"
        "${FFMPEG_DIR}/lib/avdevice.lib"
        "${FFMPEG_DIR}/lib/avutil.lib"
        "${FFMPEG_DIR}/lib/swscale.lib"
        ole32 
        winmm 
        uuid
        strmiids
        calliforniaCore
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${PORTAUDIO_DIR}/libportaudio.a"
        "${CRYPTOPP_DIR}/libcryptopp.a"
        libspdlog.a
        calliforniaCore
    )
endif()

target_include_directories(callsClientPy PRIVATE
    ${ROOT_DIR}/client/core/src
    ${ROOT_DIR}/vendor/portaudio/include
    ${ROOT_DIR}/vendor/json/include                    
    ${ROOT_DIR}/vendor/cryptopp               
    ${ROOT_DIR}/vendor/asio/asio/include
    ${ROOT_DIR}/vendor/spdlog/include
    ${ROOT_DIR}/vendor/ticTimer
)