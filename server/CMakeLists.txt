cmake_minimum_required(VERSION 3.6)
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
    "../vendor/portaudio/build/${CMAKE_BUILD_TYPE}"
    "../vendor/portaudio/build"
    "../vendor/portaudio/lib"
    "../vendor/portaudio-master/build/${CMAKE_BUILD_TYPE}"  
    "../vendor/portaudio-master/build"   
)

set(CRYPTOPP_POSSIBLE_PATHS
    "../vendor/cryptopp/x64/Output/${CMAKE_BUILD_TYPE}"
    "../vendor/cryptopp/build"
    "../vendor/cryptopp/lib"
    "../vendor/cryptopp-master/Output/${CMAKE_BUILD_TYPE}"  
    "../vendor/cryptopp-master/x64/Output/${CMAKE_BUILD_TYPE}"  
    "../vendor/cryptopp-master/build"   
)

find_library(OPUS_LIBRARY
    NAMES opus opusd libopus  
    PATHS ${OPUS_POSSIBLE_PATHS}
    NO_DEFAULT_PATH
)

find_library(PORTAUDIO_LIBRARY
    NAMES portaudio portaudio_static_x64  
    PATHS ${PORTAUDIO_POSSIBLE_PATHS}
    NO_DEFAULT_PATH
)

find_library(CRYPTOPP_LIBRARY
    NAMES cryptlib cryptlib_x64  
    PATHS ${CRYPTOPP_POSSIBLE_PATHS}
    NO_DEFAULT_PATH
)

if(NOT OPUS_LIBRARY)
    set(OPUS_LIBRARY "C:/prj/C++/Calls/vendor/opus-main/build/Debug/opus.lib")  
    message(WARNING "Using hardcoded OPUS_LIBRARY path: ${OPUS_LIBRARY}")    
endif()

if(NOT PORTAUDIO_LIBRARY)
    set(PORTAUDIO_LIBRARY "C:/prj/C++/Calls/vendor/portaudio/build/Debug/portaudio_static_x64.lib")
    message(WARNING "Using hardcoded PORTAUDIO_LIBRARY path: ${PORTAUDIO_LIBRARY}")
endif()

if(NOT CRYPTOPP_LIBRARY)
    set(CRYPTOPP_LIBRARY "C:/prj/C++/Calls/vendor/cryptopp/x64/Output/cryptlib.lib")
    message(WARNING "Using hardcoded CRYPTOPP_LIBRARY path: ${CRYPTOPP_LIBRARY}")
endif()

get_filename_component(OPUS_LIB_DIR ${OPUS_LIBRARY} DIRECTORY)
get_filename_component(PORTAUDIO_LIB_DIR ${PORTAUDIO_LIBRARY} DIRECTORY)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${OPUS_LIB_DIR}
    ${PORTAUDIO_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PORTAUDIO_LIBRARY}
    ${OPUS_LIBRARY}
    ${CRYPTOPP_LIBRARY}
    ole32 winmm uuid
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/opus-main/include     
    ../vendor/portaudio/include
    ../vendor/json/include                    
    ../vendor/cryptopp-master                    
    ../vendor/asio
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)