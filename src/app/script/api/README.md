# JavaScript API Bindings

*Last updated: 2025-12-30*

This directory contains the script bindings that expose LibreSprite's functionality to JavaScript/Lua scripts.

## Overview

LibreSprite provides a scripting API that allows users to automate tasks, create custom tools, and extend the application. These bindings use a `ScriptObject` base class pattern to expose C++ objects and methods to the scripting engine.

## Key Files

### Application API

- **app_script.cpp** - The main `app` global object. Provides access to:
  - `activeSprite`, `activeDocument`, `activeImage` - Current document state
  - `activeFrameNumber`, `activeLayerNumber` - Current frame/layer indices
  - `version`, `platform` - Application metadata
  - `createDialog()`, `openFile()`, `exit()` - Application-level operations
  - `pixelColor`, `command` - Sub-objects for color manipulation and command execution

### Document Model Bindings

- **sprite_script.cpp** - Sprite manipulation (dimensions, palettes, layers, frames)
- **document_script.cpp** - Document-level operations
- **layer_script.cpp** - Layer properties and manipulation
- **cel_script.cpp** - Cel (frame/layer intersection) access
- **image_script.cpp** - Pixel-level image manipulation
- **palette_script.cpp** - Palette color access and modification
- **selection_script.cpp** - Selection/mask operations
- **colormode_script.cpp** - Color mode constants (RGB, Indexed, Grayscale)
- **pixelcolor_script.cpp** - Color conversion utilities (rgba, rgbaR, rgbaG, etc.)

### UI Widget Bindings

- **dialog_script.cpp** - Create custom dialog windows
- **button_script.cpp** - Button widget bindings
- **label_script.cpp** - Label/text widget bindings
- **entry_script.cpp** - Text entry field bindings
- **intentry_script.cpp** - Integer entry field bindings
- **imageview_script.cpp** - Image display widget bindings
- **palettelistbox_script.cpp** - Palette list widget bindings
- **widget_script.h** - Base widget binding helpers

### Utility Bindings

- **command_script.cpp** - Execute menu commands programmatically
- **console_script.cpp** - Console/debug output
- **storage_script.cpp** - Persistent storage API for scripts

## Usage Example

```javascript
// Access the current sprite
var sprite = app.activeSprite;
print("Width: " + sprite.width + ", Height: " + sprite.height);

// Modify pixels
var image = app.activeImage;
image.putPixel(10, 10, app.pixelColor.rgba(255, 0, 0, 255));

// Create a dialog
var dlg = app.createDialog("My Dialog");
dlg.addLabel("label1", "Hello World!");
dlg.addButton("ok", "OK");
dlg.show();
```

## Architecture

Each `*_script.cpp` file defines a `ScriptObject` subclass that:
1. Registers properties via `addProperty()` with getter/setter lambdas
2. Registers methods via `addMethod()` with documentation strings
3. Uses `inject<>` for dependency injection of sub-objects
