# SDL2 Platform Backend

*Last updated: 2025-12-30*

This directory contains the SDL2 (Simple DirectMedia Layer 2) platform backend for the SHE (Simple Hardware Environment) library. SDL2 provides cross-platform windowing, rendering, and input handling.

## Overview

The SDL2 backend is the primary cross-platform implementation used by LibreSprite on Linux, Windows, and other platforms. It handles window creation, hardware-accelerated rendering, and input event processing through the SDL2 library.

## Key Components

### `SDL2Display` (sdl2_display.cpp/h)
The main display class implementing the `Display` interface:
- Window creation and management (`SDL_Window`)
- Hardware rendering via `SDL_Renderer`
- Fullscreen toggle support
- Window scaling and DPI handling
- Mouse cursor management
- GPU-accelerated rendering when available

### `SDL2Surface` (sdl2_surface.cpp/h)
Surface implementation wrapping `SDL_Surface`:
- Pixel-level drawing operations (lines, rectangles, fills)
- Blitting and compositing
- Texture caching for GPU rendering
- Integration with generic text rendering from `common/generic_surface.h`

### `SDL2System` (she.cpp)
The main system class that:
- Initializes SDL2 subsystems
- Creates displays and surfaces
- Processes input events (keyboard, mouse, tablet)
- Integrates with EasyTab for tablet support on Windows/Linux

## Files

| File | Description |
|------|-------------|
| `sdl2_display.cpp/h` | Window and display management |
| `sdl2_surface.cpp/h` | Surface/texture operations |
| `she.cpp` | Main system implementation and event loop |

## Features

- **GPU Acceleration**: Optional hardware rendering via SDL_Renderer
- **Tablet Support**: EasyTab integration for pressure-sensitive input
- **Fullscreen**: Toggle between windowed and fullscreen modes
- **Scaling**: Integer scaling for pixel-perfect rendering
- **Cross-Platform**: Works on Linux, Windows, and other SDL2-supported platforms

## Build Requirements

- SDL2 2.0.x or later
- SDL2_image (for image loading)
- EasyTab (optional, for tablet support)
