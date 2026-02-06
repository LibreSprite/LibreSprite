# Commands

*Last updated: 2025-12-30*

User-invocable commands triggered via menus, keyboard shortcuts, or the command palette.

## Architecture

Each command is a class derived from `Command` that implements `onExecute()`. Commands are registered in `CommandsModule` and invoked by ID.

## Naming Convention

Files follow the pattern `cmd_<action>.cpp`:
- `cmd_open_file.cpp` - Open file dialog
- `cmd_save_file.cpp` - Save current document
- `cmd_undo.cpp` - Undo last action
- `cmd_copy.cpp`, `cmd_paste.cpp` - Clipboard operations

## Key Commands

| File | Command |
|------|---------|
| `cmd_new_file.cpp` | Create new sprite |
| `cmd_open_file.cpp` | Open existing file |
| `cmd_save_file.cpp` | Save document |
| `cmd_export_sprite_sheet.cpp` | Export as sprite sheet |
| `cmd_canvas_size.cpp` | Resize canvas |
| `cmd_sprite_size.cpp` | Scale sprite |
| `cmd_flip.cpp` | Flip horizontally/vertically |
| `cmd_rotate.cpp` | Rotate sprite |
| `cmd_palette_editor.cpp` | Open palette editor |
| `cmd_options.cpp` | Open preferences dialog |

## Adding a New Command

1. Create `cmd_yourcommand.cpp`
2. Define a class inheriting from `Command`
3. Implement `onExecute(Context*)`
4. Register in `commands.cpp` or via `CommandsModule`
