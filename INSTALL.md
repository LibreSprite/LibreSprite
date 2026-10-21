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
  * [Android details](#android-details)
  * [Web (emscripten) details](#web-emscripten-details)
* [Installing](#installing)

## Platforms

You can download installers from the [website](https://libresprite.github.io/).
If you want to compile LibreSprite from source, continue reading.

You should be able to compile LibreSprite on the following platforms:

* Windows 10 + VS2015 Community Edition + Windows 10 SDK
* Mac OS X 11.0 Big Sur + Xcode 7.3 + OS X 11.0 SDK
* Linux + GCC 8.5 or higher with C++14 support

To compile LibreSprite you will need:

* [CMake](http://www.cmake.org/) (3.4 or greater)
* [Ninja](https://ninja-build.org)
* [Msys2](https://www.msys2.org/) (Windows only)

## Get the source code

Clone the repository and its submodules using the following command:

    git clone --recursive https://github.com/LibreSprite/LibreSprite

(You can use [Git for Windows](https://git-for-windows.github.io/) to
clone the repository on Windows.)

To update an existing clone, use the following commands:

    cd LibreSprite
    git pull
    git submodule update --init --recursive

## Dependencies

You'll need the following dependencies to compile LibreSprite:

### Linux dependencies

Debian/Ubuntu:

    sudo apt-get install cmake g++ libfreetype6-dev libgif-dev libgtest-dev libjpeg-dev libpixman-1-dev libpng-dev libsdl2-dev libsdl2-image-dev libtinyxml2-dev libnode-dev ninja-build zlib1g-dev libarchive-dev

Fedora:

    sudo dnf install g++ cmake freetype-devel giflib-devel gtest-devel libjpeg-devel pixman-devel libpng-devel SDL2-devel SDL2_image-devel tinyxml2-devel zlib-devel ninja-build nodejs-devel libarchive-devel

### Windows dependencies

To install the required dependencies with msys2, run the following in the MSYS2 UCRT64 shell:

    pacman -S base-devel mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-freetype mingw-w64-ucrt-x86_64-giflib mingw-w64-ucrt-x86_64-libjpeg-turbo mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-libwebp mingw-w64-ucrt-x86_64-pixman mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-tinyxml2 mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-libarchive

### MacOS dependencies

On MacOS you will need Mac OS X 11.0 SDK and the corresponding Xcode.
In a terminal, install the dependencies using brew:

    brew install gnutls freetype jpeg webp pixman sdl2 sdl2_image tinyxml2 libarchive ninja zlib xmlto dylibbundler cmake

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

### Windows details

Run the following in the MSYS2 UCRT64 shell:

    cmake -G Ninja ..

### MacOS details

To compile LibreSprite, run the following commands:
```
    cmake \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
      -G Ninja \
      ..
    ninja libresprite
```
### Android details

Before you can make an Android build, you must make a native build for your OS,
so follow the appropriate instructions above. Once that is done, download
https://github.com/LibreSprite/ls-android-deps as android/ in the LibreSprite
directory. Now you can open the android subdirectory in Android Studio and build
LibreSprite for Android.

The build uses the CMake from the Android SDK by default, which may be too old.
This project needs CMake 4.1 or newer. To use your own CMake instead, add this
line to `android/local.properties`, replacing the path with the directory that
contains `bin/cmake`:

    cmake.dir=/usr

### Web (emscripten) details

Before you can make a web build, you must make a native build for your OS,
so follow the appropriate instructions above. The web build reuses the
sources generated in `build/src/` and preloads the data from
`build/bin/data`, so the native build's `gen` step and the data copy must
have run first. You'll also need [emsdk](https://emsdk.org/), Ninja and
CMake 4.1 or newer.

    emcmake cmake -S emscripten -B emscripten/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    ninja -C emscripten/build

This produces `emscripten/build/libresprite.html`, `libresprite.js`,
`libresprite.wasm` and `libresprite.data`. `emscripten/index.html` loads
`libresprite.js` from the same directory, so serve the build artifacts
next to it.

The build uses pthreads, which need SharedArrayBuffer. Browsers only
allow that on a cross-origin isolated page, so serve the page over http(s)
with the `Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp` headers.

Known gotchas:

- On a fresh emsdk cache, the first link can fail with a cache assertion
  from the `sdl2_ttf` port sub-build
  ([emscripten-core/emscripten#20204](https://github.com/emscripten-core/emscripten/issues/20204)).
  Work around it by building the freetype port once before the first link:

      echo 'int main(){}' > /tmp/dummy.c
      emcc -O2 -pthread -sUSE_FREETYPE /tmp/dummy.c -o /tmp/dummy.js

  The flags must stay plain (no -flto, no -fwasm-exceptions): the port
  sub-build requests the default libfreetype.a variant. Then rerun the
  build.

## Installing

Once you've finished compiling, you can install LibreSprite by running the
following command from the `build` directory:

    ninja install
