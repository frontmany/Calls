cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(server_updater)

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
    set(SPDLOG_DIR "${ROOT_DIR}/vendor/spdlog/build/Release")
else()
    set(SPDLOG_DIR "../vendor/spdlog")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "${SPDLOG_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        spdlog.lib
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libspdlog.a
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/asio/asio/include
    ../vendor/json/include
    ../vendor/spdlog/include
)