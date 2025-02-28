# Aseprite     | Copyright (C) 2001-2016  David Capello
# LibreSprite  | Copyright (C)      2022  LibreSprite contributors

cmake_minimum_required(VERSION 3.4)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo Profile."
      FORCE)
endif()

# Restrict configuration types to the selected build type.
# Note: This needs to be done before the project command
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "internal")

# LibreSprite project
project(libresprite C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This required for KDE/Qt destop integration, which sets BUILD_SHARED_LIBS to
# TRUE by default
set(BUILD_SHARED_LIBS off)

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

option(WITH_WEBP_SUPPORT            "Enable support to load/save .webp files" off)
option(WITH_GTK_FILE_DIALOG_SUPPORT "Enable support for the experimental native GTK File Dialog" off)
option(WITH_DEPRECATED_GLIB_SUPPORT "Enable support for older glib versions" off)
option(WITH_DESKTOP_INTEGRATION     "Enable desktop integration modules" off)
option(WITH_QT_THUMBNAILER          "Enable kde5/qt5 thumnailer" off)

option(ENABLE_MEMLEAK     "Enable memory-leaks detector (only for developers)" off)
option(ENABLE_TESTS       "Enable the unit tests" off)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)

option(USE_SDL2_BACKEND "Use SDL2 backend" on)
option(USE_V8_SANDBOX "Use V8 Sandbox" off)

######################################################################
# Profile build type

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling linker flags")
endif()

if(MSVC)
    set(CMAKE_C_FLAGS_PROFILE "/MD /Zi /Ox /Gd"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "/PROFILE /DEBUG"
        CACHE STRING "Profiling linker flags")
endif()

######################################################################
# Directories

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# We need to specify the output for each configuration to make it work
# on Visual Studio solutions.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/bin")

set(EASYTAB_DIR         ${CMAKE_SOURCE_DIR}/third_party/EasyTab)
set(LOADPNG_DIR         ${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(SIMPLEINI_DIR       ${CMAKE_SOURCE_DIR}/third_party/simpleini)
set(DUKTAPE_DIR         ${CMAKE_SOURCE_DIR}/third_party/duktape)
set(MODP_B64_DIR        ${CMAKE_SOURCE_DIR}/third_party/modp_b64)
set(QOI_DIR             ${CMAKE_SOURCE_DIR}/third_party/qoi)

# Search in the "cmake" directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Put libraries into "lib".
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

######################################################################
# Common definitions to compile all sources (app code and third party)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE -D_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 -g")
else()
  add_definitions(-DNDEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
endif()

# Fix to compile gtest with VC11 (2012)
if(MSVC_VERSION EQUAL 1700)
  add_definitions(-D_VARIADIC_MAX=10)
endif()

if(NOT WIN32 AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu89")
endif()

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# qoi
include_directories(${QOI_DIR})

# tinyxml2
find_library(TINYXML2_LIBRARY NAMES tinyxml2)
find_path(TINYXML2_INCLUDE_DIR NAMES tinyxml2.h)
include_directories(${TINYXML2_INCLUDE_DIR})

if(NOT GEN_ONLY)
# zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# libArchive
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Homebrew ships libarchive keg only, include dirs have to be set manually
    execute_process(
        COMMAND brew --prefix libarchive
        OUTPUT_VARIABLE LIBARCHIVE_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(LibArchive_INCLUDE_DIR "${LIBARCHIVE_PREFIX}/include")
endif()
find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIR})

# libpng
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# libwebp
find_package(WEBP QUIET)
if(WEBP_FOUND)
  message(STATUS "Enabling WEBP support")
  include_directories(${WEBP_INCLUDE_DIR})
else()
  find_package(PkgConfig)
  if (PkgConfig_FOUND)
    pkg_search_module(WEBP libwebp)
    if (WEBP_FOUND)
      message(STATUS "Enabling WEBP support")
      include_directories(${WEBP_INCLUDE_DIRS})
      set(WEB_LIBRARIES ${WEBP_LDFLAGS})
    endif()
  endif()
endif()

# pixman
find_library(PIXMAN_LIBRARY NAMES pixman pixman-1)
find_path(PIXMAN_INCLUDE_DIR NAMES pixman.h PATH_SUFFIXES pixman-1)
include_directories(${PIXMAN_INCLUDE_DIR})

# freetype
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

find_package(GIF REQUIRED)
include_directories(${GIF_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIRS})

# v8
find_package(V8 QUIET)
if(V8_FOUND)
  include_directories(${V8_INCLUDE_DIR})
  add_definitions(-DSCRIPT_ENGINE_V8=1)

  include(CheckCXXSourceRuns)
  set(CMAKE_REQUIRED_LIBRARIES ${V8_LIBRARY} ${V8_LIBBASE_LIBRARY} ${V8_LIBPLATFORM_LIBRARY})
  set(CMAKE_REQUIRED_INCLUDES ${V8_INCLUDE_DIR})
  message(STATUS ${V8_POINTER_COMPRESSION_OUTPUT})
  # Since V8_COMPRESS_POINTERS is not defined here, the resulting program would
  # die if the system v8 library had enabled pointer compression.
  check_cxx_source_runs("
    #include <v8.h>
    #include <libplatform/libplatform.h>
    int main(void) {
      v8::V8::InitializeICU();
      static std::unique_ptr<v8::Platform> m_platform;
      m_platform = v8::platform::NewDefaultPlatform();
      v8::V8::InitializePlatform(m_platform.get());
      v8::V8::Initialize();
      return 0;
    }
	" DISABLE_POINTER_COMPRESSION)

  # pointers compression doesn't work on 32bit machines
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(DISABLE_POINTER_COMPRESSION EQUAL 1)
      message(STATUS "Disabling V8 pointer compression")
    else()
      message(STATUS "Enabling V8 pointer compression")
      add_definitions(-DV8_COMPRESS_POINTERS)
    endif()
  endif()

  if (USE_V8_SANDBOX)
    add_definitions(-DV8_ENABLE_SANDBOX)
  endif()

  message(STATUS "Optional V8 library found. Enabling V8 scripting engine.")
else()
  message(STATUS "Optional V8 library NOT found. Disabling V8 scripting engine.")
endif()


find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
endif()
# simpleini
include_directories(${SIMPLEINI_DIR})

######################################################################
# Platform specific stuff

set(PLATFORM_LIBS)

# SDL2 backend
if(USE_SDL2_BACKEND)
  add_definitions(-DUSE_SDL2_BACKEND)

  find_package(SDL2 REQUIRED)
  include_directories(${SDL2_INCLUDE_DIRS})
  find_package(SDL2_IMAGE REQUIRED)
  include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
endif()

# -- Unix --

if(UNIX AND NOT APPLE AND NOT BEOS AND NOT GEN_ONLY)
  # Pthreads
  find_package(Threads REQUIRED)
  list(APPEND PLATFORM_LIBS m ${CMAKE_THREAD_LIBS_INIT})

  option(USE_X11 "Use X11" on)
  if(USE_X11)
    # X11
    find_package(X11 REQUIRED)
    include_directories(SYSTEM ${X11_INCLUDE_DIR})
    list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})

    if(X11_XShm_FOUND)
      list(APPEND PLATFORM_LIBS ${X11_Xext_LIB})
    endif()

    if(X11_Xcursor_FOUND)
      list(APPEND PLATFORM_LIBS ${X11_Xcursor_LIB})
    endif()

    if(X11_Xpm_FOUND)
      list(APPEND PLATFORM_LIBS ${X11_Xpm_LIB})
    endif()

    find_library(X11_Xxf86vm_LIB Xxf86vm ${X11_LIB_SEARCH_PATH})
    mark_as_advanced(X11_Xxf86vm_LIB)
    if(X11_xf86vmode_FOUND)
      list(APPEND PLATFORM_LIBS ${X11_Xxf86vm_LIB})
    endif()

    check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)
    check_library_exists(Xxf86dga XDGAQueryExtension
      "${X11_LIB_SEARCH_PATH}" XDGA_FOUND)

    if(XDGA_FOUND)
      list(APPEND PLATFORM_LIBS Xxf86dga ${X11_LIBRARIES})
    endif()
  endif()

  if(X11_Xi_FOUND)
    list(APPEND PLATFORM_LIBS ${X11_Xi_LIB})
  endif()

  if(WITH_GTK_FILE_DIALOG_SUPPORT)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTKMM gtkmm-3.0)
    include_directories(${GTKMM_INCLUDE_DIRS})
    link_directories(${GTKMM_LIBRARY_DIRS})
  endif()
endif()

# -- Windows --

if(WIN32)
  list(APPEND PLATFORM_LIBS
    kernel32 user32 gdi32 comdlg32 ole32 winmm
    shlwapi psapi wininet comctl32 dbghelp)

  # Windows XP is the minimum supported platform.
  add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)

  # We need Unicode support
  add_definitions(-DUNICODE -D_UNICODE)
endif(WIN32)

# -- Mac OS X --

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  find_library(IOKIT_LIBRARY IOKit)
  mark_as_advanced(COCOA_LIBRARY CARBON_LIBRARY IOKIT_LIBRARY)

  list(APPEND PLATFORM_LIBS
    ${COCOA_LIBRARY}
    ${CARBON_LIBRARY}
    ${IOKIT_LIBRARY})

    # Hack to deal with Mac OS X 10.6.  NSQuickDrawView is not defined by
    # NSQuickDrawView.h when compiling in 64-bit mode, and 64-bit mode is the
    # default when compiling on Snow Leopard.
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i386)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")
    endif()

    # The Mac port simply uses too many deprecated things.
    if(COMPILER_GCC)
        set(WFLAGS "${WFLAGS} -Wno-deprecated-declarations")
    endif(COMPILER_GCC)
endif(APPLE)

if(WITH_DESKTOP_INTEGRATION)
  add_subdirectory(desktop)
endif()

######################################################################
# Main ASE targets

add_subdirectory(src)

######################################################################
# Third party libraries

add_subdirectory(third_party)
