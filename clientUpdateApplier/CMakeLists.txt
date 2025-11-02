cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(update_applier)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64")
else()
    set(CMAKE_PREFIX_PATH "/home/vboxuser/Qt/6.9.3/gcc_64")
endif()

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} Qt6::Core)