# LibreSprite WebAssembly Build

[![Emscripten](https://img.shields.io/badge/Emscripten-WebAssembly-6C4AB6?logo=webassembly)](https://emscripten.org/)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://isocpp.org/)
[![SDL2](https://img.shields.io/badge/SDL2-Emscripten-orange)](https://www.libsdl.org/)

This directory contains the build configuration for compiling LibreSprite to WebAssembly using Emscripten, enabling the sprite editor to run directly in web browsers.

## Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | CMake configuration for Emscripten build with WASM-specific flags and dependencies |
| `index.html` | HTML shell template that hosts the WebAssembly application with loading UI |

## Requirements

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (emsdk)
- CMake 4.1+
- Ninja (recommended) or Make
- Pre-built native LibreSprite (for generated source files)

## Building

### 1. Install Emscripten SDK

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### 2. Build Native Version First

The Emscripten build requires generated source files from a native build:

```bash
cd /path/to/LibreSprite
make build
```

This creates `build/src/app/pref.xml.cpp` and other generated files.

### 3. Build for WebAssembly

```bash
cd /path/to/LibreSprite/emscripten
mkdir -p build && cd build
emcmake cmake ..
emmake make -j$(nproc)
```

The build produces:
- `libresprite.html` - Main HTML file
- `libresprite.js` - JavaScript glue code
- `libresprite.wasm` - WebAssembly binary
- `libresprite.data` - Preloaded assets (data directory)

### 4. Serve Locally

WebAssembly requires HTTP headers that file:// URLs cannot provide:

```bash
cd build
python3 -m http.server 8080
```

Open http://localhost:8080/libresprite.html in your browser.

## Build Configuration

The Emscripten build uses these key settings:

| Setting | Value | Purpose |
|---------|-------|---------|
| `USE_PTHREADS` | 1 | Enable threading via Web Workers |
| `PTHREAD_POOL_SIZE` | 4 | Pre-spawn 4 worker threads |
| `ALLOW_MEMORY_GROWTH` | 1 | Dynamic heap allocation |
| `USE_SDL` | 2 | SDL2 port for graphics/input |
| `USE_FREETYPE` | 1 | Font rendering |
| `USE_ZLIB`, `USE_LIBPNG`, `USE_GIFLIB`, `USE_LIBJPEG` | 1 | Image format support |

## Limitations

The web version has several limitations compared to native builds:

### File System
- No direct access to local files
- Files are stored in browser's IndexedDB or temporary memory filesystem
- Import/export works through browser download/upload dialogs

### Performance
- Slightly slower than native due to WebAssembly overhead
- Large sprites may cause memory pressure
- Threading limited to SharedArrayBuffer (requires COOP/COEP headers)

### Features
- No native file dialogs
- Clipboard integration limited by browser APIs
- Some keyboard shortcuts may conflict with browser shortcuts
- No system font access (bundled fonts only)

### Browser Requirements
- Modern browser with WebAssembly support
- SharedArrayBuffer support (requires secure context with COOP/COEP headers)
- WebGL 2.0 for hardware-accelerated rendering

## Server Headers

For threading support, your web server must send these headers:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Without these headers, the application falls back to single-threaded mode.

---

![Repobeats analytics](https://repobeats.axiom.co/api/embed/6c5c8d6a5f8c7b8e9a0b1c2d3e4f5a6b7c8d9e0f.svg "Repobeats analytics image")
