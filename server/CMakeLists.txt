cmake_minimum_required(VERSION 3.16)
project(calls_server VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

set(OPUS_POSSIBLE_PATHS
    "../vendor/opus-main/build/${CMAKE_BUILD_TYPE}"
    "../vendor/opus-main/build"
    "../vendor/opus-main/lib"
)

set(PORTAUDIO_POSSIBLE_PATHS
    "../vendor/portaudio-master/build/${CMAKE_BUILD_TYPE}"
    "../vendor/portaudio-master/build"
    "../vendor/portaudio-master/lib"
)

find_library(OPUS_LIBRARY
    NAMES opus opusd
    PATHS ${OPUS_POSSIBLE_PATHS}
    NO_DEFAULT_PATH
)

find_library(PORTAUDIO_LIBRARY
    NAMES portaudio portaudiod
    PATHS ${PORTAUDIO_POSSIBLE_PATHS}
    NO_DEFAULT_PATH
)

get_filename_component(OPUS_LIB_DIR ${OPUS_LIBRARY} DIRECTORY)
get_filename_component(PORTAUDIO_LIB_DIR ${PORTAUDIO_LIBRARY} DIRECTORY)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${OPUS_LIB_DIR}
    ${PORTAUDIO_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPUS_LIBRARY}
    ${PORTAUDIO_LIBRARY}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/opus-main/include
    ../vendor/portaudio-master/include
    ../vendor/asio
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
