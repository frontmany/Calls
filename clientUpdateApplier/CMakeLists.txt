cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(updateApplier)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(GLOB SOURCES 
    "src/*.cpp" 
    "src/*.h"
    "src/utilities/*.cpp"
    "src/utilities/*.h"
)
source_group("Source Files" FILES ${SOURCES})

# Настраиваемый путь к Qt6
# Можно задать через:
#   cmake -DQt6_DIR=/path/to/Qt6/lib/cmake/Qt6
#   cmake -DCMAKE_PREFIX_PATH=/path/to/Qt6
#   или через переменную окружения CMAKE_PREFIX_PATH
if(WIN32)
    # Значение по умолчанию для Windows (можно переопределить через -DCMAKE_PREFIX_PATH=...)
    if("${CMAKE_PREFIX_PATH}" STREQUAL "")
        set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64" CACHE PATH "Qt6 prefix path")
    endif()
else()
    # Для Linux: проверяем переменную окружения или используем значение по умолчанию
    if(DEFINED ENV{CMAKE_PREFIX_PATH})
        set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}" CACHE PATH "Qt6 prefix path")
    elseif("${CMAKE_PREFIX_PATH}" STREQUAL "")
        # Значение по умолчанию для Linux (можно переопределить через -DCMAKE_PREFIX_PATH=...)
        set(CMAKE_PREFIX_PATH "/home/vboxuser/Qt/6.9.3/gcc_64" CACHE PATH "Qt6 prefix path")
    endif()
endif()

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SOURCES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0601
    )
endif()

if(WIN32)
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build")

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
else()
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")

    target_link_directories(${PROJECT_NAME} PRIVATE
        "${SPDLOG_DIR}"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        libspdlog.a
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOT_DIR}/vendor/spdlog/include
    ${ROOT_DIR}/vendor/CrashCatch/include
)