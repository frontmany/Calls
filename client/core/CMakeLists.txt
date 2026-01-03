cmake_minimum_required(VERSION 3.6)
project(calls_client VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_library(${PROJECT_NAME} ${SOURCES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0601
        ASIO_STANDALONE
    )
endif()

set(OPUS_DIR "${ROOT_DIR}/vendor/opus/build")
set(PORTAUDIO_DIR "${ROOT_DIR}/vendor/portaudio/build")

if (WIN32) 
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp/x64/Output")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
else()
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp")
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "${PORTAUDIO_DIR}"
    "${OPUS_DIR}"
    "${CRYPTOPP_DIR}"
    "${SPDLOG_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${PORTAUDIO_DIR}/$<CONFIG>/portaudio_static_x64.lib"
        "${OPUS_DIR}/$<CONFIG>/opus.lib"
        "${CRYPTOPP_DIR}/$<CONFIG>/cryptlib.lib"
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
        ole32 
        winmm 
        uuid
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${PORTAUDIO_DIR}/libportaudio.a"
        "${OPUS_DIR}/libopus.a"
        "${CRYPTOPP_DIR}/libcryptopp.a"
        libspdlog.a
    )
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    ${ROOT_DIR}/vendor/opus/include     
    ${ROOT_DIR}/vendor/portaudio/include
    ${ROOT_DIR}/vendor/json/include                    
    ${ROOT_DIR}/vendor/cryptopp               
    ${ROOT_DIR}/vendor/spdlog/include              
    ${ROOT_DIR}/vendor/asio/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)