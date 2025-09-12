@echo off
setlocal enabledelayedexpansion
echo [INFO] Building PortAudio for MSVC with Visual Studio Generator...
set PORTAUDIO_DIR=./vendor/portaudio
set BUILD_DIR=%PORTAUDIO_DIR%/build
set OUTPUT_DEBUG_DIR=%PORTAUDIO_DIR%/build/Debug
set OUTPUT_RELEASE_DIR=%PORTAUDIO_DIR%/build/Release

REM Проверяем существует ли директория portaudio
if not exist "%PORTAUDIO_DIR%" (
    echo [ERROR] PortAudio directory not found: %PORTAUDIO_DIR%
    pause
    exit /b 1
)

REM Удаляем старую сборку если существует
if exist "%BUILD_DIR%" (
    echo [INFO] Removing old build directory...
    rmdir /s /q "%BUILD_DIR%"
)

REM Создаем директорию для сборки
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo [INFO] Configuring Debug build...
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Debug ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING="MultiThreadedDebugDLL" ^
    -DCMAKE_C_FLAGS:STRING="/MDd" ^
    -DCMAKE_CXX_FLAGS:STRING="/MDd" ^
    -DPA_USE_DS=OFF ^
    -DPA_USE_WDMKS=OFF ^
    -DPA_USE_WASAPI=ON ^
    --no-warn-unused-cli

if errorlevel 1 (
    echo [ERROR] CMake configuration for Debug failed.
    pause
    exit /b 1
)

echo [INFO] Building Debug version...
cmake --build . --config Debug

if errorlevel 1 (
    echo [ERROR] Build for Debug failed.
    pause
    exit /b 1
)

echo [INFO] Debug build completed successfully!

echo [INFO] Configuring Release build...
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING="MultiThreadedDLL" ^
    -DCMAKE_C_FLAGS:STRING="/MD" ^
    -DCMAKE_CXX_FLAGS:STRING="/MD" ^
    -DPA_USE_DS=OFF ^
    -DPA_USE_WDMKS=OFF ^
    -DPA_USE_WASAPI=ON ^
    --no-warn-unused-cli

if errorlevel 1 (
    echo [ERROR] CMake configuration for Release failed.
    pause
    exit /b 1
)

echo [INFO] Building Release version...
cmake --build . --config Release

if errorlevel 1 (
    echo [ERROR] Build for Release failed.
    pause
    exit /b 1
)

echo [INFO] Release build completed successfully!
pause