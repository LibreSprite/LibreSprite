# Installation

## Table of contents

* [Platforms](#platforms)
* [Get the source code](#get-the-source-code)
* [Dependencies](#dependencies)
  * [Linux dependencies](#linux-dependencies)
  * [Windows dependencies](#windows-dependencies)
  * [MacOS dependencies](#macos-dependencies)
* [Compiling](#compiling)
  * [Linux details](#linux-details)
  * [Windows details](#windows-details)
  * [MacOS details](#macos-details)
* [Installing](#installing)

## Platforms

You can download installers from the [website](https://libresprite.github.io/).
If you want to compile LibreSprite from source, continue reading.

You should be able to compile LibreSprite on the following platforms:

* Windows 10 + VS2015 Community Edition + Windows 10 SDK
* Mac OS X 10.11.4 El Capitan + Xcode 7.3 + OS X 10.11 SDK
* Linux + GCC 5.2 with C++14 support

To compile LibreSprite you will need:

* [CMake](http://www.cmake.org/) (3.4 or greater)
* [Ninja](https://ninja-build.org)
* [vcpkg](https://vcpkg.io/en/getting-started.html) (Windows and MacOS only)

## Get the source code

Clone the repository and its submodules using the following command:

    git clone --recursive https://github.com/LibreSprite/LibreSprite

(You can use [Git for Windows](https://git-for-windows.github.io/) to
clone the repository on Windows.)

To update an existing clone, use the following commands:

    cd LibreSprite
    git pull
    git submodule update --init --recursive

## Backends

LibreSprite can be compiled with two different backends:

1. SDL2 backend (WIP)
2. Allegro backend (deprecated) (Windows and Linux only)

## Dependencies

You'll need the following dependencies to compile LibreSprite:

### Linux dependencies

Debian/Ubuntu:

    sudo apt-get install cmake g++ libcurl4-gnutls-dev libfreetype6-dev libgif-dev libgtest-dev libjpeg-dev liblua5.4-dev libpixman-1-dev libpng-dev libsdl2-dev libsdl2-image-dev libtinyxml-dev ninja-build zlib1g-dev

### Windows dependencies

To install the required dependencies with vcpkg, run:

    vcpkg install curl freetype giflib gtest libjpeg-turbo lua libpng libwebp pixman sdl2 sdl2-image tinyxml zlib --triplet x64-windows

Beware: `--triplet x64-windows` is only necessary for a 64 architecture
build.

### MacOS dependencies

On MacOS you will need Mac OS X 10.11 SDK and Xcode 7.3 (older versions
might work).

## Compiling

First, create the `build` directory with the following commands:

    cd LibreSprite
    mkdir build
    cd build

Then following the platform-specific instructions for compiling below.

The `build` directory will contain the results of the compilation process.
If you want to build a fresh copy of LibreSprite, remove the `build` directory
and recompile.

### Linux details

To compile LibreSprite, run the following commands:

    cmake -G Ninja ..
    ninja libresprite

To compile the legacy Allegro backend, run `cmake` with the flags
`-DUSE_SDL2_BACKEND=off -DUSE_ALLEG4_BACKEND=on`.

The repository contains a patched version of the Allegro 4 library.
If you want to use your installed version of Allegro, run `cmake` with
the flag `-DUSE_SHARED_ALLEGRO4=ON`. However, this is not recommended due to
issues with Allegro 4.4
[(1)](https://github.com/aseprite/aseprite/issues/192)
[(2)](https://github.com/LibreSprite/LibreSprite/commit/27b55030e26e93c5e8d9e7e21206c8709d46ff22).

### Windows details

If you're using a command prompt and aiming for a x64 build, be sure to
use the [x64 prompt](https://i.stack.imgur.com/qeR0b.png) or it won't
find `vcpkg` libraries.

To compile LibreSprite, run the following commands:

    cmake ^
      -DCMAKE_TOOLCHAIN_FILE=put_your_vcpkg_path_here\vcpkg\scripts\buildsystems\vcpkg.cmake ^
      -G Ninja ^
      ..

To compile the legacy Allegro backend, run `cmake` with the flags
`-DUSE_SDL2_BACKEND=off -DUSE_ALLEG4_BACKEND=on`.

The repository contains a patched version of the Allegro 4 library.
If you want to use your installed version of Allegro, run `cmake` with
the flag `-DUSE_SHARED_ALLEGRO4=on`.

### MacOS details

To compile LibreSprite, run the following commands:

    cmake \
      -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk \
      -G Ninja \
      ..
    ninja libresprite

## Installing

Once you've finished compiling, you can install LibreSprite by running the
following command from the `build` directory:

    ninja install
