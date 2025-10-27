cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(callifornia)

set(Vulkan_FOUND TRUE)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/6.9.2/msvc2022_64")
find_package(Qt6 COMPONENTS Widgets Core Multimedia REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

qt6_add_resources(QRC_FILES resources/resources.qrc)
set(RC_FILE resources/app_icon.rc)

add_executable(${PROJECT_NAME} ${SOURCES} ${QRC_FILES} ${RC_FILE})

target_link_libraries(${PROJECT_NAME} Qt6::Widgets Qt6::Core Qt6::Multimedia calls_client client_updater)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ../client/src
    ../clientUpdater/src
    ../vendor/opus/include     
    ../vendor/portaudio/include
    ../vendor/json/include                    
    ../vendor/cryptopp               
    ../vendor/asio/asio/include
    )