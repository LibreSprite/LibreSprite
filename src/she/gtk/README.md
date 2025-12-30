# GTK Platform Backend

*Last updated: 2025-12-30*

This directory contains the GTK3 platform backend for the SHE (Simple Hardware Environment) library. It provides native Linux desktop integration through GTKmm (the C++ bindings for GTK).

## Overview

The GTK backend is specifically used to provide native file dialogs on Linux systems. When LibreSprite is compiled with `ASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT`, this backend enables native GTK file open/save dialogs instead of the application's built-in dialogs.

## Components

### `NativeDialogsGTK3` (native_dialogs.cpp/h)
Implements the `NativeDialogs` interface using GTK3:
- Creates native file open/save dialogs via `Gtk::Application`
- Provides familiar Linux desktop experience for file operations
- Integrates with the system's file manager and recent files

## Files

| File | Description |
|------|-------------|
| `native_dialogs.cpp` | GTK3 file dialog implementation |
| `native_dialogs.h` | NativeDialogsGTK3 class declaration |

## Build Requirements

- GTKmm 3.0 or later
- GLib development headers
- Compile flag: `ASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT`

## Note

This backend only handles native dialogs. The main windowing and rendering on Linux is handled by the SDL2 backend (`src/she/sdl2/`).
