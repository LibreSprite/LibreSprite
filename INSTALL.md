# Installation

## Quick Start (Recommended)

Use the Makefile for easy builds:

```bash
make deps      # Install dependencies
make build     # Build LibreSprite
make run       # Run LibreSprite
make dmg       # Create distributable DMG (macOS)
```

Run `make help` for all available commands.

## Table of Contents

* [Platforms](#platforms)
* [Get the source code](#get-the-source-code)
* [Quick build with Make](#quick-build-with-make)
* [Manual build](#manual-build)
  * [Dependencies](#dependencies)
  * [Compiling](#compiling)
  * [Installing](#installing)

## Platforms

Download installers from the [website](https://libresprite.github.io/), or compile from source.

Supported platforms:
* **macOS** 11.0+ with Xcode
* **Linux** with GCC 8.5+ (C++14 support)
* **Windows** 10+ with VS2015 or Msys2

Requirements:
* [CMake](http://www.cmake.org/) 3.4+
* [Ninja](https://ninja-build.org)
* [Msys2](https://www.msys2.org/) (Windows only)

## Get the Source Code

```bash
git clone --recursive https://github.com/LibreSprite/LibreSprite
cd LibreSprite
```

To update an existing clone:

```bash
git pull
git submodule update --init --recursive
```

## Quick Build with Make

The Makefile handles all platform-specific details automatically:

| Command | Description |
|---------|-------------|
| `make deps` | Install dependencies (brew/apt/dnf) |
| `make build` | Configure and compile |
| `make run` | Run LibreSprite |
| `make bundle` | Create macOS .app bundle |
| `make dmg` | Create distributable DMG (macOS) |
| `make clean` | Remove build directory |
| `make distclean` | Remove build, bundle, and DMG |

**Examples:**
```bash
make deps build run     # First time setup
make build run          # Rebuild and run
make dmg                # Build → bundle → DMG (auto-chains)
```

## Manual Build

If you prefer manual control or the Makefile doesn't work for your setup.

### Dependencies

**macOS:**
```bash
brew install gnutls freetype jpeg webp pixman sdl2 sdl2_image tinyxml2 \
  libarchive v8 ninja zlib xmlto dylibbundler cmake create-dmg
```

**Debian/Ubuntu:**
```bash
sudo apt-get install cmake g++ libcurl4-gnutls-dev libfreetype6-dev \
  libgif-dev libgtest-dev libjpeg-dev libpixman-1-dev libpng-dev \
  libsdl2-dev libsdl2-image-dev libtinyxml2-dev libnode-dev ninja-build \
  zlib1g-dev libarchive-dev
```

**Fedora:**
```bash
sudo dnf install g++ cmake libcurl-devel freetype-devel giflib-devel \
  gtest-devel libjpeg-devel pixman-devel libpng-devel SDL2-devel \
  SDL2_image-devel tinyxml2-devel zlib-devel ninja-build nodejs-devel \
  libarchive-devel
```

**Windows (Msys2 mingw32):**
```bash
pacman -S base-devel mingw-w64-i686-gcc mingw-w64-i686-cmake \
  mingw-w64-i686-make mingw-w64-i686-curl mingw-w64-i686-freetype \
  mingw-w64-i686-giflib mingw-w64-i686-libjpeg-turbo mingw-w64-i686-libpng \
  mingw-w64-i686-libwebp mingw-w64-i686-pixman mingw-w64-i686-SDL2 \
  mingw-w64-i686-SDL2_image mingw-w64-i686-tinyxml2 mingw-w64-i686-v8 \
  mingw-w64-i686-zlib mingw-w64-i686-libarchive
```

### Compiling

**macOS:**
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
ninja -C build
```

**Linux:**
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

**Windows (Msys2):**
```bash
cmake -S . -B build -G Ninja
ninja -C build
```

### Installing

```bash
ninja -C build install
```

### Android

First complete a native build for your OS, then:
1. Download [ls-android-deps](https://github.com/LibreSprite/ls-android-deps) as `android/`
2. Open the `android/` directory in Android Studio
3. Build from Android Studio
