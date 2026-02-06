# Windows Platform Backend

*Last updated: 2025-12-30*

This directory contains the Windows platform backend for the SHE (Simple Hardware Environment) library. It provides native Windows integration using the Win32 API.

## Overview

The Windows backend implements native windowing, input handling, and system integration for LibreSprite on Windows. It uses Win32 APIs directly for window management and input processing, with Wintab support for professional drawing tablets.

## Key Components

### `WinWindow` (window.h)
Template-based window class implementing native Win32 windowing:
- Window creation using `CreateWindowEx`
- Message handling via window procedure (`WndProc`)
- Mouse and keyboard input processing
- Tablet pressure and pointer type detection
- DDE (Dynamic Data Exchange) for file associations

### `NativeDialogsWin32` (native_dialogs.cpp/h)
Native Windows file dialogs using the Common Item Dialog API:
- Open file dialogs
- Save file dialogs
- Modern Vista-style dialogs when available

### `PenAPI` (pen.cpp/h)
Wintab API wrapper for professional tablet support:
- Wacom and compatible tablet integration
- Pressure sensitivity
- Cursor type detection (pen, eraser, etc.)
- Based on Wacom's Wintab specification

### Window DDE (window_dde.cpp/h)
Dynamic Data Exchange support for:
- Opening files via shell association
- Inter-process communication

## Files

| File | Description |
|------|-------------|
| `window.h` | WinWindow template class for Win32 windowing |
| `native_dialogs.cpp/h` | Native file dialog implementation |
| `pen.cpp/h` | Wintab tablet API wrapper |
| `vk.cpp` | Virtual key to scancode mapping |
| `event_queue.h` | Windows event queue integration |
| `system.h` | Windows system class declarations |
| `window_dde.cpp/h` | DDE protocol support |

## Features

- **Native Win32**: Direct Windows API usage for optimal performance
- **Tablet Support**: Full Wintab API integration for pressure-sensitive tablets
- **DDE Integration**: Opens files when LibreSprite is launched via file association
- **Modern Dialogs**: Uses Common Item Dialog (Vista+) for file operations

## Build Requirements

- Windows SDK
- Visual Studio or MinGW toolchain
- Wintab SDK headers (included in `wacom/` directory)
