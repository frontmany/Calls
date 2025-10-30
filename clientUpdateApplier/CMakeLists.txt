cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(update_applier)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/6.9.2/msvc2022_64")
find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} Qt6::Core)