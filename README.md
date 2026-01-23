# Build Instructions

## Prerequisites

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
