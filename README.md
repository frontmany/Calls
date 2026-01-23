# Build Instructions

## Prerequisites

### Qt6 Setup

The project requires Qt6 for building the UI components. The Qt path is configurable and can be set in several ways:

**For Windows:**
- Default path: `C:/Qt/6.9.3/msvc2022_64`
- Can be overridden via CMake: `-DCMAKE_PREFIX_PATH=C:/path/to/Qt/6.9.3/msvc2022_64`

**For Linux:**
- Default path: `/home/vboxuser/Qt/6.9.3/gcc_64`
- Can be overridden via:
  - CMake: `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.3/gcc_64`
  - Environment variable: `export CMAKE_PREFIX_PATH=/path/to/Qt/6.9.3/gcc_64`
  - Or: `-DQt6_DIR=/path/to/Qt/6.9.3/gcc_64/lib/cmake/Qt6`

## Build for Windows (MSVC)

* clone the repository


* run fetch_dependencies.bat file
(this will run the cmake which will install all the dependencies)

**Or manually configure CMake with Qt:**

```bash
# Fetch dependencies with custom Qt path
cmake -DFETCH_DEPENDENCIES=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64 -B build

# Or just configure for build
cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64 -B build
cmake --build build --config Debug
```

## Build for Linux

**Fetch dependencies and build:**

```bash
# With default Qt path
cmake -DFETCH_DEPENDENCIES=ON -B build
cmake --build build

# With custom Qt path via CMake variable
cmake -DFETCH_DEPENDENCIES=ON -DCMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64 -B build
cmake --build build

# With custom Qt path via environment variable
export CMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64
cmake -DFETCH_DEPENDENCIES=ON -B build
cmake --build build

# Or using Qt6_DIR (more precise)
cmake -DFETCH_DEPENDENCIES=ON -DQt6_DIR=/opt/Qt/6.9.3/gcc_64/lib/cmake/Qt6 -B build
cmake --build build
```

**Just build (dependencies already fetched):**

```bash
# With default Qt path
cmake -B build
cmake --build build

# With custom Qt path
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/6.9.3/gcc_64 -B build
cmake --build build
```


* run build_opus.bat and build_portaudio.bat files
(this bat files will buld all configurations debug/release of a certain libraries)


* however there is still one library you should build manually. Go to vendor/cryptopp folder and find cryptest.sln, open it and (super important) - go to cryptlib project properties, then  C/C++, then Code generation and set up runtime libraty to MDd or MD. If you not do that, your project wouldn't link. Then build both configurations of cryptlib debug with MDd runtime /release with MD runtime 


* run genDebug.bat or genRelease.bat depends on what configuration you want. Go to vendor\json\include\nlohmann and copy the json.hpp file one directory up to vendor\json\include


---
After these five steps you may go to build directory and open calls.sln
