# CMake Modules

This directory contains custom CMake find modules for locating dependencies that are not included in CMake's default module set or require LibreSprite-specific configuration.

## Find Modules

| Module | Dependency | Platform | Description |
|--------|------------|----------|-------------|
| `FindDXGuid.cmake` | DirectX GUID library | Windows | Locates dxguid library from DirectX SDK for Windows builds |
| `FindSDL2.cmake` | SDL2 | All | Locates SDL2 library and headers; prefers pkg-config, falls back to manual search |
| `FindSDL2_IMAGE.cmake` | SDL2_image | All | Locates SDL2_image extension library for image loading support |
| `FindTests.cmake` | Google Test | All | Utility module that discovers and registers `*_tests.cpp` files with CTest |
| `FindV8.cmake` | V8 JavaScript engine | All | Locates V8 library for optional JavaScript scripting support |

## How Modules Are Used

The root `CMakeLists.txt` adds this directory to the module search path:

```cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
```

After this, modules are invoked via `find_package()`:

```cmake
find_package(SDL2 REQUIRED)
find_package(SDL2_IMAGE REQUIRED)
find_package(V8 QUIET)  # Optional dependency
```

The `FindTests.cmake` module provides the `find_tests()` function used in `src/CMakeLists.txt`:

```cmake
find_tests(base base-lib)
find_tests(gfx gfx-lib)
```

## Adding New Find Modules

1. Create `Find<PackageName>.cmake` in this directory
2. Define standard output variables:
   - `<PACKAGE>_FOUND` - Boolean indicating if the package was found
   - `<PACKAGE>_INCLUDE_DIRS` - Header file directories
   - `<PACKAGE>_LIBRARIES` - Libraries to link against
   - `<PACKAGE>_VERSION_STRING` - Version if detectable (optional)
3. Use `find_package_handle_standard_args()` for consistent behavior
4. Call via `find_package(<PackageName>)` in CMakeLists.txt

Example template:

```cmake
find_path(<PACKAGE>_INCLUDE_DIR <header>.h
    HINTS ENV <PACKAGE>DIR
    PATH_SUFFIXES include
)

find_library(<PACKAGE>_LIBRARY
    NAMES <libname>
    HINTS ENV <PACKAGE>DIR
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(<PACKAGE> DEFAULT_MSG
    <PACKAGE>_LIBRARY <PACKAGE>_INCLUDE_DIR)

if(<PACKAGE>_FOUND)
    set(<PACKAGE>_LIBRARIES ${<PACKAGE>_LIBRARY})
    set(<PACKAGE>_INCLUDE_DIRS ${<PACKAGE>_INCLUDE_DIR})
endif()

mark_as_advanced(<PACKAGE>_INCLUDE_DIR <PACKAGE>_LIBRARY)
```
