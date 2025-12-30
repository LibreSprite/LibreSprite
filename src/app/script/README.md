# Scripting

*Last updated: 2025-12-30*

JavaScript scripting API for automating LibreSprite.

## Architecture

Uses Duktape JavaScript engine. Scripts can access application objects via a bound API defined in the `api/` subdirectory.

## Key Classes

| Class | Purpose |
|-------|---------|
| `AppScripting` | Main scripting engine initialization |
| `ConsoleDelegate` | Handles script console output |
| `ScriptMenu` | Manages scripts available in the Scripts menu |

## Files

| File | Purpose |
|------|---------|
| `app_scripting.cpp/h` | Initialize scripting engine and bindings |
| `console_delegate.cpp` | Script output handling |
| `script_menu.cpp/h` | Scripts menu population |
| `api/` | JavaScript API bindings (app, sprite, layer, etc.) |

## Script Locations

Scripts are loaded from:
- `data/scripts/` - Bundled scripts
- User profile `scripts/` directory - User scripts

## Example Script

```javascript
// Access active sprite
var sprite = app.activeSprite;

// Modify the sprite
sprite.width = 64;
sprite.height = 64;
```
