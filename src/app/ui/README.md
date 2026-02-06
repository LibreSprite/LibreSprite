# Application UI

*Last updated: 2025-12-30*

LibreSprite-specific UI widgets and dialogs.

## Overview

These widgets extend the base `ui/` library with application-specific functionality for sprite editing, color selection, animation timeline, etc.

## Key Widgets

| Widget | Purpose |
|--------|---------|
| `ColorBar` | Color palette and foreground/background color selector |
| `ColorButton` | Button showing a color with picker popup |
| `ColorSelector` | Color wheel/spectrum picker |
| `Timeline` | Animation frames and layers timeline |
| `Editor` | Main sprite canvas with zoom/pan |
| `StatusBar` | Bottom status bar with coordinates, zoom |
| `Toolbar` | Left tool palette |
| `ContextBar` | Top context-sensitive tool options |
| `Workspace` | Tabbed document container |
| `PaletteView` | Color palette grid display |

## Dialogs

| File | Dialog |
|------|--------|
| `color_popup.cpp` | Color picker popup |
| `brush_popup.cpp` | Brush selector popup |
| `palette_popup.cpp` | Palette selector popup |
| `keyboard_shortcuts.cpp` | Keyboard shortcuts editor |

## Editor Components

The `editor/` subdirectory contains components for the main canvas:
- `EditorState` - State machine for editor modes (standby, drawing, scrolling)
- `EditorView` - Scrollable container for editor
- `Zoom` - Zoom level management
- `PixelsMovement` - Selection move/transform handling
