@echo off
setlocal enabledelayedexpansion

echo [INFO] Building Opus for Mingw with Ninja...

set OPUS_DIR=./vendor/opus-main
set BUILD_DIR=%OPUS_DIR%/build
set OUTPUT_DEBUG_DIR=%OPUS_DIR%/build/Debug
set OUTPUT_RELEASE_DIR=%OPUS_DIR%/build/Release

REM Проверяем существует ли директория portaudio
if not exist "%OPUS_DIR%" (
    echo [ERROR] Opus directory not found: %OPUS_DIR%
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
    -DCMAKE_BUILD_TYPE=Debug ^
    -DPA_USE_DS=OFF ^
    -DPA_USE_WDMKS=OFF ^
    -DPA_USE_WASAPI=ON ^
    -DCMAKE_STATIC_LIBRARY_SUFFIX=.lib ^
    -DCMAKE_SHARED_LIBRARY_SUFFIX=.dll

echo [INFO] Building Debug version...
cmake --build . --config Debug

echo [INFO] Debug build completed successfully!


echo [INFO] Configuring Release build...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DPA_USE_DS=OFF ^
    -DPA_USE_WDMKS=OFF ^
    -DPA_USE_WASAPI=ON ^
    -DCMAKE_STATIC_LIBRARY_SUFFIX=.lib ^
    -DCMAKE_SHARED_LIBRARY_SUFFIX=.dll

echo [INFO] Building Release version...
cmake --build . --config Release

echo [INFO] Release build completed successfully!

pause