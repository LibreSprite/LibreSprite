# V8 JavaScript Engine Integration

*Last updated: 2025-12-30*

This directory contains the V8 JavaScript engine integration for LibreSprite's scripting system.

## Overview

[V8](https://v8.dev/) is Google's high-performance JavaScript and WebAssembly engine, used in Chrome and Node.js. This integration provides JavaScript scripting capabilities for LibreSprite with optimized JIT compilation for better performance on complex scripts.

## Key Components

### `engine.cpp`

The main implementation file containing:

- **`V8Engine`** - The primary engine class that:
  - Initializes the V8 platform and isolate
  - Creates execution contexts for script evaluation
  - Evaluates JavaScript code via `eval()`
  - Raises events to JavaScript handlers via `raiseEvent()`
  - Manages memory with low memory notifications

- **`V8ScriptObject`** - Bridge class for exposing C++ objects to JavaScript:
  - Converts V8 values to LibreSprite `Value` types
  - Exposes C++ functions as callable JavaScript functions
  - Manages object properties with getters/setters
  - Uses weak references for automatic cleanup when JavaScript objects are garbage collected

## Features

- High-performance JIT-compiled JavaScript execution
- External startup data support (`snapshot_blob.bin`) for faster initialization
- Type conversion between JavaScript and C++ (numbers, strings, booleans, Uint8Arrays, objects)
- Property descriptor support with getter/setter functions
- Global object registration for API exposure
- Stack trace support for debugging errors
- Compatibility layer for V8 major version differences (v7 vs v8+)

## Registration

The engine registers itself with the identifier `"js"` and handles `.js` file extensions:

```cpp
static Engine::Regular<V8Engine> registration("js", {"js"});
```

## Build Requirement

This engine is conditionally compiled when `SCRIPT_ENGINE_V8` is defined:

```cpp
#if SCRIPT_ENGINE_V8
// ... implementation
#endif
```

## Dependencies

- V8 library headers (`v8.h`, `libplatform/libplatform.h`)
- LibreSprite base libraries (memory, exceptions, conversions, filesystem)
- LibreSprite scripting framework (`script/engine.h`, `script/engine_delegate.h`, `script/value.h`)
- Resource finder for locating V8 snapshot files
