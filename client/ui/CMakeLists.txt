cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(callifornia)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

if(NOT TARGET calliforniaCore AND EXISTS "${ROOT_DIR}/client/core/CMakeLists.txt")
    add_subdirectory("${ROOT_DIR}/client/core" "${CMAKE_BINARY_DIR}/client/core")
endif()

if(NOT TARGET calliforniaUpdater AND EXISTS "${ROOT_DIR}/client/updater/CMakeLists.txt")
    add_subdirectory("${ROOT_DIR}/client/updater" "${CMAKE_BINARY_DIR}/client/updater")
endif()

set(Vulkan_FOUND TRUE)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.h"
)
source_group("Source Files" FILES ${SOURCES})

if(WIN32)
    # Значение по умолчанию для Windows
    set(QT6_DEFAULT_PATH "C:/Qt/6.9.3/msvc2022_64")
else()
    # Для Linux: проверяем переменную окружения или используем значение по умолчанию
    if(DEFINED ENV{CMAKE_PREFIX_PATH})
        set(QT6_DEFAULT_PATH "$ENV{CMAKE_PREFIX_PATH}")
    else()
        # Значение по умолчанию для Linux
        set(QT6_DEFAULT_PATH "/opt/Qt/6.9.3/gcc_64")
    endif()
endif()

# Добавляем путь к Qt в CMAKE_PREFIX_PATH если его там еще нет
if(CMAKE_PREFIX_PATH)
    list(FIND CMAKE_PREFIX_PATH "${QT6_DEFAULT_PATH}" QT6_PATH_INDEX)
    if(QT6_PATH_INDEX EQUAL -1)
        list(APPEND CMAKE_PREFIX_PATH "${QT6_DEFAULT_PATH}")
    endif()
else()
    set(CMAKE_PREFIX_PATH "${QT6_DEFAULT_PATH}")
endif()

find_package(Qt6 COMPONENTS Widgets Core Multimedia REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

qt6_add_resources(QRC_FILES resources/resources.qrc)
set(RC_FILE resources/app_icon.rc)

if(WIN32)
    set(RC_FILE resources/app_icon.rc)
    add_executable(${PROJECT_NAME} ${SOURCES} ${QRC_FILES} ${RC_FILE})
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
    
    # Check for hardware encoder support
    find_path(CUDA_INCLUDE_DIR cuda_runtime.h
        PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA*/include"
        DOC "CUDA include directory"
    )
    
    find_library(CUDA_LIBRARY cuda
        PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA*/lib/x64"
        DOC "CUDA library"
    )
    
    find_library(NVENC_LIBRARY nvEncodeAPI64
        PATHS "C:/Windows/System32" "C:/Program Files/NVIDIA Corporation/NVENC SDK*/lib/x64"
        DOC "NVENC library"
    )
    
    if(CUDA_INCLUDE_DIR AND CUDA_LIBRARY AND NVENC_LIBRARY)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_NVENC)
        target_include_directories(${PROJECT_NAME} PRIVATE ${CUDA_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${CUDA_LIBRARY} ${NVENC_LIBRARY})
        message(STATUS "NVENC support enabled")
    endif()
    
    # Intel QSV support
    find_path(QSV_INCLUDE_DIR mfxvideo++.h
        PATHS "C:/Program Files (x86)/Intel/Media SDK/*/include"
        DOC "Intel Media SDK include directory"
    )
    
    find_library(QSV_LIBRARY libmfx
        PATHS "C:/Program Files (x86)/Intel/Media SDK/*/lib/x64"
        DOC "Intel Media SDK library"
    )
    
    if(QSV_INCLUDE_DIR AND QSV_LIBRARY)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_QSV)
        target_include_directories(${PROJECT_NAME} PRIVATE ${QSV_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${QSV_LIBRARY})
        message(STATUS "Intel QSV support enabled")
    endif()
endif()

if(WIN32)
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")
    set(OPENH264_DIR "${ROOT_DIR}/vendor/openh264/builddir")
else()
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")
    set(OPENH264_DIR "${ROOT_DIR}/vendor/openh264")
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Multimedia
        calliforniaCore
        calliforniaUpdater
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${OPENH264_DIR}"
    )
else()
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${SPDLOG_DIR}"
        "${OPENH264_DIR}"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Multimedia calliforniaCore calliforniaUpdater libspdlog.a)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOT_DIR}/client/core/src
    ${ROOT_DIR}/client/updater/src
    ${ROOT_DIR}/vendor/opus/include     
    ${ROOT_DIR}/vendor/portaudio/include
    ${ROOT_DIR}/vendor/json/include                    
    ${ROOT_DIR}/vendor/cryptopp               
    ${ROOT_DIR}/vendor/asio/asio/include
    ${ROOT_DIR}/vendor/spdlog/include
    ${ROOT_DIR}/vendor/ticTimer
    ${ROOT_DIR}/vendor/CrashCatch/include
    ${ROOT_DIR}/vendor/openh264/codec/api/wels
    ${ROOT_DIR}/vendor/openh264/codec/common/inc
    )

if(WIN32)
    # Windows: link against .lib files
    target_link_libraries(${PROJECT_NAME} PRIVATE
        openh264
    )
    
    # Copy OpenH264 DLL to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/vendor/openh264/builddir/openh264-8.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying OpenH264 DLL to output directory"
    )
else()
    # Linux: link against shared libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        openh264
    )
endif()