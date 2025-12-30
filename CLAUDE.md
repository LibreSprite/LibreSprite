# LibreSprite - Claude Code Context

*Last updated: 2025-12-30*

> **IMPORTANT**: Don't commit and push without explicit user consent.

## Building

```bash
make deps      # Install dependencies (brew/apt/dnf)
make build     # Build with CMake + Ninja
make run       # Run the app
make bundle    # Create .app bundle (macOS)
make dmg       # Create distributable DMG (macOS)
make clean     # Clean build artifacts
```

Run `make help` for all commands.

## Project Overview

LibreSprite is a free, open-source sprite editor and animation tool. Forked from Aseprite (GPL v2) before it went proprietary. C++ codebase using SDL2 for cross-platform support.

**Key concepts**: Sprites, Layers, Frames, Cels (layer+frame intersection), Palettes, Tools, Commands

## Architecture (Layered Modules)

```
Level 5: main/          → Entry point, bootstraps app
Level 4: app/           → Main application (commands, tools, UI, documents)
Level 3: filters/       → Image effects
         render/        → Document rendering
         ui/            → Widget library (buttons, windows, etc.)
Level 2: doc/           → Document model (Sprite, Layer, Frame, Image)
         she/           → Platform abstraction (SDL2, Win32, Cocoa)
Level 1: cfg/           → INI file handling
         gen/           → Code generation from XML
Level 0: base/          → Core utilities (threading, filesystem, memory)
         gfx/           → Graphics primitives (Point, Rect, Color)
         fixmath/       → Fixed-point math
         clip/          → Clipboard
         undo/          → Undo/redo history
         script/        → JavaScript engine (Duktape/V8)
```

## Key Entry Points

| File | Purpose |
|------|---------|
| `src/main/main.cpp` | `app_main()` - application entry |
| `src/app/app.cpp` | `App` class - initialization, main loop |
| `src/app/context.cpp` | `Context` - active document, tool state |
| `src/app/ui/editor/editor.cpp` | Main canvas where sprites are edited |

## Directory Guide

| Directory | Purpose |
|-----------|---------|
| `src/app/commands/` | User commands (Open, Save, Undo, etc.) - menu/keyboard actions |
| `src/app/cmd/` | Undoable document modifications (internal) |
| `src/app/tools/` | Drawing tools (pencil, brush, fill, selection) |
| `src/app/ui/` | Application widgets (timeline, color bar, toolbar) |
| `src/app/ui/editor/` | Main canvas with state machine for interactions |
| `src/app/file/` | File format loaders/savers (PNG, GIF, ASE, etc.) |
| `src/app/script/` | JavaScript scripting API |
| `src/doc/` | Document model (Sprite, Layer, Cel, Image, Palette) |
| `src/she/` | Platform backends (SDL2, Win32, Cocoa, GTK) |
| `src/ui/` | Generic UI widget library |
| `src/render/` | Sprite rendering with zoom, onion skinning |
| `data/` | Runtime assets (skins, palettes, scripts, widgets) |

## Patterns & Conventions

### Commands vs Cmd
- **Command** (`src/app/commands/`) - User-invocable actions (menus, keyboard shortcuts)
  - Inherit from `Command`, implement `onExecute(Context*)`
  - Files: `cmd_*.cpp` (e.g., `cmd_open_file.cpp`)
- **Cmd** (`src/app/cmd/`) - Undoable document modifications
  - Used internally by Commands for undo support

### Editor State Machine
The editor (`src/app/ui/editor/`) uses states for different modes:
- `StandbyState` - Default, waiting for input
- `DrawingState` - Active tool drawing
- `ScrollingState` - Panning the canvas
- `MovingPixelsState` - Transforming selection

### Preferences
Defined in `data/pref.xml`, code-generated into typed C++ accessors.
Access via `App::instance()->preferences()`.

### Naming Conventions
- **Classes**: `PascalCase` (e.g., `SpriteEditor`, `ColorBar`)
- **Files**: `snake_case.cpp/h` (e.g., `color_bar.cpp`)
- **Methods**: `camelCase` (e.g., `getActiveDocument()`)

### Signals/Observers
Uses observable pattern from `base/signal.h` for event notification.

## Testing

Tests use Google Test. Test files have `_tests.cpp` suffix and live alongside source.
Include `src/tests/test.h` for test infrastructure.

## Debugging

- **Windows**: F5 shows memory usage, Ctrl+Shift+Q triggers crash (for testing recovery)
- **Crash recovery**: `src/app/crash/` handles session backup/restore

## Data Files

| Path | Purpose |
|------|---------|
| `data/gui.xml` | Main window layout |
| `data/pref.xml` | Preference definitions (code-generated) |
| `data/skins/` | UI themes (sheet.png + skin.xml) |
| `data/palettes/` | Color palettes |
| `data/scripts/` | Bundled JavaScript scripts |
| `data/widgets/` | Dialog layouts |

## Adding Features

### New Command
1. Create `src/app/commands/cmd_yourfeature.cpp`
2. Inherit from `Command`
3. Implement `onExecute(Context*)`, optionally `onEnabled()`, `onChecked()`
4. Add to `data/gui.xml` for menu placement

### New Tool
1. Add tool definition to `src/app/tools/tool_box.cpp`
2. Implement ink, controller, point shape as needed
3. Add icon to skin sheet

### New Preference
1. Add to `data/pref.xml`
2. Rebuild (code generation happens automatically)
3. Access via `preferences().section().option()`

## References

Each `src/` subdirectory has a README.md with detailed documentation.
See `src/README.md` for the full module dependency graph.
