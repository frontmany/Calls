cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(callifornia)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

if(NOT TARGET calls_client AND EXISTS "${ROOT_DIR}/client/CMakeLists.txt")
    add_subdirectory("${ROOT_DIR}/client" "${CMAKE_BINARY_DIR}/client")
endif()

if(NOT TARGET client_updater AND EXISTS "${ROOT_DIR}/clientUpdater/CMakeLists.txt")
    add_subdirectory("${ROOT_DIR}/clientUpdater" "${CMAKE_BINARY_DIR}/clientUpdater")
endif()

set(Vulkan_FOUND TRUE)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64" CACHE PATH "Qt6 path" FORCE)
else()
    set(CMAKE_PREFIX_PATH "/home/vboxuser/Qt/6.9.3/gcc_64" CACHE PATH "Qt6 path" FORCE)
endif()

find_package(Qt6 COMPONENTS Widgets Core Multimedia REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

qt6_add_resources(QRC_FILES resources/resources.qrc)
set(RC_FILE resources/app_icon.rc)

if(WIN32)
    set(RC_FILE resources/app_icon.rc)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${QRC_FILES} ${RC_FILE})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${QRC_FILES})
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0601
        ASIO_STANDALONE
    )
endif()

if(WIN32)
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
else()
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Multimedia
        calls_client
        client_updater
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
else()
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${SPDLOG_DIR}"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Multimedia calls_client client_updater libspdlog.a)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
    ../client/src
    ../clientUpdater/src
    ../vendor/opus/include     
    ../vendor/portaudio/include
    ../vendor/json/include                    
    ../vendor/cryptopp               
    ../vendor/asio/asio/include
    ../vendor/spdlog/include
    )