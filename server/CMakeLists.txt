cmake_minimum_required(VERSION 3.6)
project(calls_server VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group("Source Files" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})


if(WIN32)
    set(CRYPTOPP_DIR "${ROOT_DIR}/vendor/cryptopp/x64/Output")
else()
    set(CRYPTOPP_DIR "../vendor/cryptopp")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    "${CRYPTOPP_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${CRYPTOPP_DIR}/$<CONFIG>/cryptlib.lib"
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libcryptopp.a
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ../vendor/json/include                    
    ../vendor/cryptopp              
    ../vendor/asio/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)