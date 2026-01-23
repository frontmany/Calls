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
#   cmake -DCMAKE_PREFIX_PATH=/path/to/Qt6
#   или через переменную окружения CMAKE_PREFIX_PATH
#   (альтернативно: cmake -DQt6_DIR=/path/to/Qt6/lib/cmake/Qt6)
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