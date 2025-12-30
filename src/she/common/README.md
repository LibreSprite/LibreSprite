# Common Platform Abstraction Code

*Last updated: 2025-12-30*

This directory contains shared, platform-independent code for the SHE (Simple Hardware Environment) library. These components are used across all platform backends (Windows, macOS, GTK, SDL2) to provide consistent functionality.

## Overview

The SHE library abstracts platform-specific functionality like windowing, input handling, and rendering. This `common` directory contains reusable implementations that work across all supported platforms.

## Key Components

### `CommonSystem` (system.h)
The base system class that provides cross-platform implementations for:
- Native file dialogs (delegating to platform-specific implementations)
- Font loading (sprite sheet and TrueType fonts)
- Logging (with macOS-specific logger support)

### `GenericSurface` (generic_surface.h)
Template-based surface rendering utilities including:
- **`GenericDrawColoredRgbaSurface`** - Draws RGBA surfaces with foreground/background color tinting
- **`GenericDrawTextSurface`** - Text rendering for both sprite sheet fonts and TrueType fonts
- Alpha blending implementation using the standard Porter-Duff compositing formula

### Font Support
- **`FreeTypeFont`** (freetype_font.cpp/h) - TrueType/OpenType font wrapper using FreeType library
- **`SpriteSheetFont`** (sprite_sheet_font.h) - Bitmap font support using sprite sheets

## Files

| File | Description |
|------|-------------|
| `system.h` | CommonSystem class with cross-platform system features |
| `generic_surface.h` | Template classes for surface drawing and text rendering |
| `freetype_font.cpp/h` | FreeType-based font loading and rendering |
| `sprite_sheet_font.h` | Sprite sheet based bitmap font support |

## Architecture

```
CommonSystem (common/system.h)
    |
    +-- NativeDialogsWin32 (win/)
    +-- NativeDialogsOSX (osx/)
    +-- NativeDialogsGTK3 (gtk/)
```

Platform backends extend these common components with platform-specific implementations.
