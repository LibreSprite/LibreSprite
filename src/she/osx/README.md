# macOS Platform Backend

*Last updated: 2025-12-30*

This directory contains the macOS (Cocoa) platform backend for the SHE (Simple Hardware Environment) library. It provides native macOS integration using Objective-C++ and the Cocoa framework.

## Overview

The macOS backend implements full native windowing, input handling, and system integration for LibreSprite on Apple platforms. It uses NSWindow, NSView, and other Cocoa APIs to provide a native macOS experience.

## Key Components

### Application Layer
- **`OSXApp`** (app.h/mm) - macOS application lifecycle management, handles `NSApplication` initialization and activation
- **`OSXAppDelegate`** (app_delegate.h/mm) - Application delegate for handling app-level events like file drops and termination

### Window Management
- **`OSXWindow`** (window.h/mm) - Native `NSWindow` wrapper with scaling, cursor management, and coordinate handling
- **`OSXWindowDelegate`** (window_delegate.h) - Window delegate for resize/close events
- **`OSXView`** (view.h/mm) - Custom `NSView` subclass handling all input and rendering

### Input Handling
- **Keyboard**: Full key event handling with modifier key support (`vk.mm` provides virtual key to scancode mapping)
- **Mouse**: Click, drag, scroll wheel, and tracking area support
- **Tablet**: Wacom tablet pressure sensitivity (`tablet.mm`)
- **Gestures**: Magnify/pinch gestures for zooming (`magnify_gesture.mm`)

### System Integration
- **`NativeDialogsOSX`** (native_dialogs.h/mm) - Native file open/save dialogs using `NSSavePanel` and `NSOpenPanel`
- **Event Queue** (event_queue.h/mm) - macOS-specific event queue integration
- **Drag & Drop** (generate_drop_files.h) - File drop handling from Finder
- **Logging** (logger.mm) - macOS unified logging support

## Files

| File | Description |
|------|-------------|
| `app.h/mm` | OSXApp class for application management |
| `app_delegate.h/mm` | NSApplicationDelegate implementation |
| `event_queue.h/mm` | Event queue for macOS |
| `generate_drop_files.h` | Drag-and-drop file extraction |
| `logger.mm` | macOS logging support |
| `magnify_gesture.mm` | Pinch-to-zoom gesture handling |
| `native_dialogs.h/mm` | Native file dialogs |
| `tablet.mm` | Wacom tablet support |
| `view.h/mm` | Custom NSView for rendering and input |
| `vk.mm` | Virtual key to scancode mapping |
| `window.h/mm` | NSWindow wrapper |
| `window_delegate.h` | Window event delegate |

## Build Requirements

- macOS SDK 10.9 or later
- Xcode with Objective-C++ support
- Cocoa framework
