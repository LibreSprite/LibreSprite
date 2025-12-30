# Main Module

*Last updated: 2025-12-30*

Application entry point for LibreSprite.

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Contains `app_main()` - initializes the system and runs the app |
| `resources_win32.rc` | Windows resource file (icons, manifest) |
| `settings.manifest` | Windows application manifest |

## Startup Sequence

1. Set locale to `en-US` for consistent number parsing
2. Initialize random seed
3. Parse command-line options via `AppOptions`
4. Create the platform abstraction layer (`she::System`)
5. Initialize memory dump handling for crash reports
6. Create and initialize the `App` instance
7. Run the main event loop

## Dependencies

Depends on: app, base, she
