cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(server_updater)

if(NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

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

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "$<IF:$<CONFIG:Debug>,${SPDLOG_DIR}/Debug/spdlogd.lib,${SPDLOG_DIR}/Release/spdlog.lib>"
    )
else()
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog")

    target_link_directories(${PROJECT_NAME} PRIVATE
        "${SPDLOG_DIR}"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        libspdlog.a
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/asio/asio/include
    ../vendor/json/include
    ../vendor/spdlog/include
)