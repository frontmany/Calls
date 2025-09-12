@echo off
setlocal enabledelayedexpansion

echo [INFO] Building Opus for Mingw with Ninja...

set OPUS_DIR=./vendor/opus
set BUILD_DIR=%OPUS_DIR%/build
set OUTPUT_DEBUG_DIR=%OPUS_DIR%/build/Debug
set OUTPUT_RELEASE_DIR=%OPUS_DIR%/build/Release

if not exist "%OPUS_DIR%" (
    echo [ERROR] Opus directory not found: %OPUS_DIR%
    pause
    exit /b 1
)

if exist "%BUILD_DIR%" (
    echo [INFO] Removing old build directory...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo [INFO] Configuring Debug build...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING="MultiThreadedDebugDLL" ^
    -DCMAKE_C_FLAGS:STRING="/MDd" ^
    -DCMAKE_CXX_FLAGS:STRING="/MDd" ^
    -DOPUS_BUILD_SHARED_LIBRARY=OFF ^
    -DOPUS_BUILD_TESTING=OFF

echo [INFO] Building Debug version...
cmake --build . --config Debug

echo [INFO] Debug build completed successfully!


echo [INFO] Configuring Release build...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING="MultiThreadedDLL" ^
    -DCMAKE_C_FLAGS:STRING="/MD" ^
    -DCMAKE_CXX_FLAGS:STRING="/MD" ^
    -DOPUS_BUILD_SHARED_LIBRARY=OFF ^
    -DOPUS_BUILD_TESTING=OFF

echo [INFO] Building Release version...
cmake --build . --config Release 

echo [INFO] Release build completed successfully!

pause