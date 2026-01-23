# Build Instructions

## Prerequisites

### Required Tools

- **Build Essentials** - C/C++ compiler and build tools. Install with:
  - Ubuntu/Debian: `sudo apt-get install build-essential`
  - Fedora/RHEL: `sudo dnf groupinstall "Development Tools"` or `sudo dnf install gcc gcc-c++ make`
  - Arch: `sudo pacman -S base-devel`

- **CMake** (version 3.16 or higher) - Build system generator. Install with:
  - Ubuntu/Debian: `sudo apt-get install cmake`
  - Fedora/RHEL: `sudo dnf install cmake`
  - Arch: `sudo pacman -S cmake`

- **Git** - Required for fetching dependencies. Install with:
  - Ubuntu/Debian: `sudo apt-get install git`
  - Fedora/RHEL: `sudo dnf install git`
  - Arch: `sudo pacman -S git`

- **OpenGL development libraries** - Required for Qt6Widgets. Install with:
  - Ubuntu/Debian: `sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev`
  - Fedora/RHEL: `sudo dnf install mesa-libGL-devel mesa-libGLU-devel`
  - Arch: `sudo pacman -S mesa glu`

- **Python development headers** - Required for pybind11 (Python bindings). Install with:
  - Ubuntu/Debian: `sudo apt-get install python3-dev`
  - Fedora/RHEL: `sudo dnf install python3-devel`
  - Arch: `sudo pacman -S python`

### Qt6 Setup

**Important:** Qt6 must be installed separately before building. The `FETCH_DEPENDENCIES` option does not install Qt.

The project requires Qt6 for building the UI components. The Qt path is configurable and can be set in several ways:

**For Windows:**
- Default path: `C:/Qt/6.9.3/msvc2022_64`
- Can be overridden via CMake: `-DCMAKE_PREFIX_PATH=C:/path/to/Qt/6.9.3/msvc2022_64`

**For Linux:**
- Default path: `/opt/Qt/6.9.3/gcc_64`
- Can be overridden via:
  - CMake: `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.3/gcc_64`
  - Environment variable: `export CMAKE_PREFIX_PATH=/path/to/Qt/6.9.3/gcc_64`
  
  *Note: If `CMAKE_PREFIX_PATH` doesn't work (e.g., non-standard Qt installation), you can use `-DQt6_DIR=/path/to/Qt/6.9.3/gcc_64/lib/cmake/Qt6` instead.*

## Before Building

**Important:** Before starting the build, you need to:

1. **Move `json.hpp` file:**
   - **Windows:** Move `vendor\json\include\nlohmann\json.hpp` to `vendor\json\include\json.hpp`
   - **Linux:** Move `vendor/json/include/nlohmann/json.hpp` to `vendor/json/include/json.hpp`

   ```bash
   # Windows (PowerShell)
   Move-Item vendor\json\include\nlohmann\json.hpp vendor\json\include\json.hpp
   
   # Linux
   mv vendor/json/include/nlohmann/json.hpp vendor/json/include/json.hpp
   ```

2. **Build portaudio and opus libraries:**
   
   **For Linux:**
   ```bash
   # Build portaudio
   cd vendor/portaudio
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   cd ../../..
   
   # Build opus
   cd vendor/opus
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   cd ../../..
   ```
   
   **For Windows:**
   ```bash
   # Build portaudio (Debug and Release)
   cd vendor\portaudio
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Debug
   cmake --build . --config Release
   cd ..\..\..
   
   # Build opus (Debug and Release)
   cd vendor\opus
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Debug
   cmake --build . --config Release
   cd ..\..\..
   ```
   
   Libraries will be placed in:
   - `vendor/portaudio/build/Debug/portaudio_static_x64.lib` and `vendor/portaudio/build/Release/portaudio_static_x64.lib`
   - `vendor/opus/build/Debug/opus.lib` and `vendor/opus/build/Release/opus.lib`

3. **Build cryptopp library:**
   
   **For Linux:**
   ```bash
   cd vendor/cryptopp
   make
   ```
   
   Library will be placed in: `vendor/cryptopp/libcryptopp.a`
   
   **For Windows:**
   ```bash
   # Open cryptest.sln in Visual Studio
   # Build cryptlib project for both Debug and Release configurations (x64 platform)
   # Or use MSBuild from command line:
   cd vendor\cryptopp
   msbuild cryptest.sln /p:Configuration=Debug /p:Platform=x64 /t:cryptlib
   msbuild cryptest.sln /p:Configuration=Release /p:Platform=x64 /t:cryptlib
   ```
   
   **Important:** Before building, you need to configure the cryptlib project:
   - Open `vendor\cryptopp\cryptest.sln` in Visual Studio
   - Right-click on `cryptlib` project → Properties
   - Go to C/C++ → Code Generation
   - Set Runtime Library to:
     - **Debug:** Multi-threaded Debug DLL (`/MDd`)
     - **Release:** Multi-threaded DLL (`/MD`)
   - Build both Debug and Release configurations
   
   Libraries will be placed in:
   - `vendor/cryptopp/x64/Output/Debug/cryptlib.lib`
   - `vendor/cryptopp/x64/Output/Release/cryptlib.lib`

4. **Build spdlog library:**
   
   **For Linux:**
   ```bash
   cd vendor/spdlog
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   cd ../..
   ```
   
   Library will be placed in: `vendor/spdlog/libspdlog.a`
   
   **For Windows:**
   ```bash
   # Build spdlog (Debug and Release)
   cd vendor\spdlog
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Debug
   cmake --build . --config Release
   cd ..\..
   ```
   
   Libraries will be placed in:
   - `vendor/spdlog/build/Debug/spdlogd.lib`
   - `vendor/spdlog/build/Release/spdlog.lib`

## Build for Windows (MSVC)

* clone the repository

**First build (fetch dependencies):**

```bash
# Fetch dependencies (Qt must be installed separately)
cmake -DFETCH_DEPENDENCIES=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64 -B build
cmake --build build --config Debug
```

**Subsequent builds (dependencies already fetched):**

```bash
# Configure and build
cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64 -B build
cmake --build build --config Debug
```

## Build for Linux

**Note:** It is recommended to exclude Python wrappers from the build on Linux to avoid issues with Position Independent Code (PIC) requirements. Python wrappers require all static libraries (opus, portaudio, cryptopp, spdlog) to be compiled with `-fPIC`, which may require rebuilding these dependencies.

To exclude Python wrappers, comment out the following lines in `CMakeLists.txt`:
```cmake
# add_subdirectory(clientCorePythonWrapper)
# add_subdirectory(serverPythonWrapper)
```

**First build (fetch dependencies):**

```bash
# With default Qt path (Qt must be installed separately)
cmake -DFETCH_DEPENDENCIES=ON -B build
cmake --build build

# With custom Qt path via CMake variable
cmake -DFETCH_DEPENDENCIES=ON -DCMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64 -B build
cmake --build build

# With custom Qt path via environment variable
export CMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64
cmake -DFETCH_DEPENDENCIES=ON -B build
cmake --build build
```

**Subsequent builds (dependencies already fetched):**

```bash
# With default Qt path
cmake -B build
cmake --build build

# With custom Qt path
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64 -B build
cmake --build build
```
