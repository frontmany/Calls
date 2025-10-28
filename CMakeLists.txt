cmake_minimum_required(VERSION 3.16)
project(Calls VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(DEPENDENCIES_FETCHED "Set to TRUE if dependencies are already fetched" FALSE)

if(NOT DEPENDENCIES_FETCHED)
    message(STATUS "Dependencies not fetched yet, including FetchDependencies.cmake")
    set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
    set(CMAKE_PREFIX_PATH ${VENDOR_DIR} ${CMAKE_PREFIX_PATH})

    message(STATUS "Fetching dependencies...")

    FetchContent_Declare(
        opus
        GIT_REPOSITORY https://github.com/xiph/opus.git
        GIT_TAG v1.4
        SOURCE_DIR ${VENDOR_DIR}/opus
    )

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        SOURCE_DIR ${VENDOR_DIR}/json
    )

    FetchContent_Declare(
        cryptopp
        GIT_REPOSITORY https://github.com/weidai11/cryptopp.git
        GIT_TAG CRYPTOPP_8_9_0
        SOURCE_DIR ${VENDOR_DIR}/cryptopp
    )

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
        SOURCE_DIR ${VENDOR_DIR}/portaudio
    )

    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
        SOURCE_DIR ${VENDOR_DIR}/asio
    )

    FetchContent_MakeAvailable(opus nlohmann_json cryptopp portaudio asio)

    set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build shared library" FORCE)
    set(OPUS_BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)

    set(CRYPTOPP_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
    set(CRYPTOPP_BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    set(CRYPTOPP_BUILD_BENCHMARKS OFF CACHE BOOL "Build benchmarks" FORCE)

    set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library" FORCE)
    set(PA_BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)

    message(STATUS "Dependencies fetched successfully")

else()
    message(STATUS "Dependencies already fetched")
    add_subdirectory(callifornia)
    add_subdirectory(server)
    add_subdirectory(client)
    add_subdirectory(clientUpdater)
    add_subdirectory(clientUpdaterExample)
    add_subdirectory(clientUpdaterPy)
    add_subdirectory(clientUpdateApplier)
    add_subdirectory(serverUpdater)
    add_subdirectory(serverUpdaterPy)
    add_subdirectory(serverPy)
    add_subdirectory(clientPy)
    
endif()