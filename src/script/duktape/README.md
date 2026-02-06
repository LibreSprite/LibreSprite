# Duktape JavaScript Engine Integration

*Last updated: 2025-12-30*

This directory contains the Duktape JavaScript engine integration for LibreSprite's scripting system.

## Overview

[Duktape](https://duktape.org/) is a lightweight, embeddable JavaScript engine designed for portability and compact footprint. This integration provides JavaScript scripting capabilities for LibreSprite as an alternative to the V8 engine.

## Key Components

### `engine.cpp`

The main implementation file containing:

- **`DukEngine`** - The primary engine class that:
  - Creates and manages the Duktape heap with custom memory allocation
  - Evaluates JavaScript code via `eval()`
  - Raises events to JavaScript handlers via `raiseEvent()`
  - Handles fatal errors and exceptions gracefully

- **`DukScriptObject`** - Bridge class for exposing C++ objects to JavaScript:
  - Converts JavaScript values to LibreSprite `Value` types
  - Exposes C++ functions as callable JavaScript functions
  - Manages object properties with getters/setters
  - Handles garbage collection and reference counting

## Features

- Custom memory allocator integration with LibreSprite's memory management
- Type conversion between JavaScript and C++ (numbers, strings, booleans, objects, buffers)
- Property getter/setter support for JavaScript objects
- Global object registration for API exposure
- Event system for bidirectional communication

## Registration

The engine registers itself with the identifier `"duk"` and handles `.js` file extensions:

```cpp
static Engine::Regular<DukEngine> registration("duk", {"js"});
```

## Dependencies

- Duktape library headers (`duk_config.h`, `duktape.h`)
- LibreSprite base libraries (memory, exceptions, conversions)
- LibreSprite scripting framework (`script/engine.h`, `script/engine_delegate.h`)
