cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
project(server_updater)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}  SQLiteCpp sqlite3)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/asio/asio/include
    ../vendor/json/include
)