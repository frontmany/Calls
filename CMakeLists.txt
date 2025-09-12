cmake_minimum_required(VERSION 3.16)
project(Calls VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEPENDENCIES_FETCHED "Set to TRUE if dependencies are already fetched" FALSE)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

if(NOT DEPENDENCIES_FETCHED)
    message(STATUS "Dependencies not fetched yet, including FetchDependencies.cmake")
    include(FetchDependencies.cmake)
else()
    message(STATUS "Dependencies already fetched, skipping FetchDependencies.cmake")
endif()

add_subdirectory(server)
add_subdirectory(client)