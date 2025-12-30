# App Library

*Last updated: 2025-12-30*

The main application logic for LibreSprite. This is the highest-level module that orchestrates all other components.

## Key Classes

- `App` - Singleton application instance managing initialization, main loop, and shutdown
- `Document` - Represents an open sprite document with undo history
- `Context` - Application context providing access to active documents and tools

## Subdirectories

| Directory | Purpose |
|-----------|---------|
| `cmd/` | Undoable document modification commands (see cmd/README.md) |
| `commands/` | User-invocable menu/keyboard commands (Open, Save, etc.) |
| `crash/` | Crash recovery and session backup system |
| `file/` | File format loaders/savers (PNG, GIF, ASE, etc.) |
| `modules/` | Legacy module initialization (see modules/README.md) |
| `pref/` | User preferences management |
| `res/` | Resource loading (palettes, brushes) |
| `script/` | JavaScript scripting API |
| `tools/` | Drawing tools (pencil, brush, fill, selection, etc.) |
| `ui/` | Application-specific UI widgets |
| `util/` | Utility functions (clipboard, autocrop, etc.) |

## Key Files

| File | Purpose |
|------|---------|
| `app.cpp/h` | Main App class - entry point for initialization |
| `app_menus.cpp/h` | Menu bar construction and management |
| `app_options.cpp/h` | Command-line argument parsing |
| `document.cpp/h` | Document class with undo support |
| `context.cpp/h` | Application context (active document, tool) |
| `transaction.cpp/h` | Transactional document modifications |

## Dependencies

Depends on: base, doc, filters, fixmath, gfx, render, script, she, ui, undo
