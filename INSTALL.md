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

    sudo apt-get install cmake g++ libcurl4-gnutls-dev libfreetype6-dev libgif-dev libgtest-dev libjpeg-dev libpixman-1-dev libpng-dev libsdl2-dev libsdl2-image-dev libtinyxml2-dev libnode-dev ninja-build zlib1g-dev libarchive-dev

Fedora:

    sudo dnf install g++ cmake libcurl-devel freetype-devel giflib-devel gtest-devel libjpeg-devel pixman-devel libpng-devel SDL2-devel SDL2_image-devel tinyxml2-devel zlib-devel ninja-build nodejs-devel libarchive-devel

### Windows dependencies

To install the required dependencies with msys2, run the following in mingw32:

    pacman -S base-devel mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-make mingw-w64-i686-curl mingw-w64-i686-freetype mingw-w64-i686-giflib mingw-w64-i686-libjpeg-turbo mingw-w64-i686-libpng mingw-w64-i686-libwebp mingw-w64-i686-pixman mingw-w64-i686-SDL2 mingw-w64-i686-SDL2_image mingw-w64-i686-tinyxml2 mingw-w64-i686-v8 mingw-w64-i686-zlib mingw-w64-i686-libarchive

### MacOS dependencies

On MacOS you will need Mac OS X 11.0 SDK and the corresponding Xcode.
In a terminal, install the dependencies using brew:

    brew install gnutls freetype jpeg webp pixman sdl2 sdl2_image tinyxml2 libarchive v8 ninja zlib xmlto dylibbundler cmake create-dmg

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

Run the following in mingw32.exe:

    cmake -G Ninja ..

### MacOS details

To compile LibreSprite, run the following commands from the project root:
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
ninja -C build
```

To create a distributable DMG:
```
# Clean up any previous bundle
rm -rf bundle libresprite.dmg

# Create app bundle structure
# IMPORTANT: Data goes in Resources (NOT MacOS) with a symlink back.
# macOS 15+ code signing fails if non-code files are in Contents/MacOS.
mkdir -p bundle/libresprite.app/Contents/{MacOS,Resources,libs}
cp build/bin/libresprite bundle/libresprite.app/Contents/MacOS/
cp -r build/bin/data bundle/libresprite.app/Contents/Resources/
cd bundle/libresprite.app/Contents/MacOS && ln -s ../Resources/data data && cd -
cp desktop/Info.plist bundle/libresprite.app/Contents/

# Generate icon
mkdir -p bundle/libresprite.iconset
sips -z 16 16 data/icons/ase64.png --out bundle/libresprite.iconset/icon_16x16.png
sips -z 32 32 data/icons/ase64.png --out bundle/libresprite.iconset/icon_16x16@2x.png
sips -z 32 32 data/icons/ase64.png --out bundle/libresprite.iconset/icon_32x32.png
sips -z 64 64 data/icons/ase64.png --out bundle/libresprite.iconset/icon_32x32@2x.png
sips -z 128 128 data/icons/ase64.png --out bundle/libresprite.iconset/icon_128x128.png
sips -z 256 256 data/icons/ase64.png --out bundle/libresprite.iconset/icon_128x128@2x.png
sips -z 256 256 data/icons/ase64.png --out bundle/libresprite.iconset/icon_256x256.png
sips -z 512 512 data/icons/ase64.png --out bundle/libresprite.iconset/icon_256x256@2x.png
sips -z 512 512 data/icons/ase64.png --out bundle/libresprite.iconset/icon_512x512.png
sips -z 512 512 data/icons/ase64.png --out bundle/libresprite.iconset/icon_512x512@2x.png
iconutil -c icns bundle/libresprite.iconset -o bundle/libresprite.app/Contents/Resources/libresprite.icns

# Bundle dynamic libraries
dylibbundler -od -b -ns -x ./bundle/libresprite.app/Contents/MacOS/libresprite \
  -d ./bundle/libresprite.app/Contents/libs/

# Sign all components (inside-out order is CRITICAL)
# 1. Sign each dylib individually
# 2. Sign the executable
# 3. Sign the bundle
# DO NOT use --deep, it does not work on macOS 15+
for lib in ./bundle/libresprite.app/Contents/libs/*.dylib; do
  codesign --force -s - "$lib"
done
codesign --force -s - ./bundle/libresprite.app/Contents/MacOS/libresprite
codesign --force -s - ./bundle/libresprite.app

# Create DMG (requires: brew install create-dmg)
create-dmg \
  --volname "LibreSprite" \
  --window-pos 200 120 \
  --window-size 600 400 \
  --icon-size 100 \
  --icon "libresprite.app" 150 185 \
  --app-drop-link 450 185 \
  "libresprite.dmg" \
  "./bundle/libresprite.app"
```
### Android details

Before you can make an Android build, you must make a native build for your OS,
so follow the appropriate instructions above. Once that is done, download
https://github.com/LibreSprite/ls-android-deps as android/ in the LibreSprite
directory. Now you can open the android subdirectory in Android Studio and build
LibreSprite for Android.


## Installing

Once you've finished compiling, you can install LibreSprite by running the
following command from the `build` directory:

    ninja install
