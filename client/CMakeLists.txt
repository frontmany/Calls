cmake_minimum_required(VERSION 3.6)
project(calls_client VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

set(OPUS_DIR "${ROOT_DIR}/vendor/opus/build")
set(PORTAUDIO_DIR "${ROOT_DIR}/vendor/portaudio/build")
set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp/x64/Output")

target_link_directories(${PROJECT_NAME} PRIVATE
    "${PORTAUDIO_DIR}"
    "${OPUS_DIR}"
    "${CRYPTOPP_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    "${PORTAUDIO_DIR}/$<CONFIG>/portaudio_static_x64.lib"
    "${OPUS_DIR}/$<CONFIG>/opus.lib"
    "${CRYPTOPP_DIR}/$<CONFIG>/cryptlib.lib"
    ole32 
    winmm 
    uuid
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/opus/include     
    ../vendor/portaudio/include
    ../vendor/json/include                    
    ../vendor/cryptopp               
    ../vendor/asio/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)