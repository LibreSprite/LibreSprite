# Icons

This directory contains application and document icons used for platform integration.

## Icon Types

| Icon | Purpose |
|------|---------|
| `ase*` | Application icon (LibreSprite executable) |
| `doc*` | Document icon (sprite files opened with LibreSprite) |

## Available Sizes

PNG files are provided at standard sizes:
- 16x16 (`*16.png`)
- 32x32 (`*32.png`)
- 48x48 (`*48.png`)
- 64x64 (`*64.png`)

ICO files (`ase.ico`, `doc.ico`) bundle multiple sizes for Windows.

## Usage

### Windows
Icons are embedded via `src/main/resources_win32.rc`:
- `ase.ico` - Application executable icon
- `doc.ico` - File association icon for sprite documents

### macOS
The Makefile generates `.icns` bundles from `ase64.png`, scaling it to all required sizes (16x16 through 512x512@2x).

### Linux
Snap packages use `ase48.png`. Desktop entries reference icons from `desktop/icons/`.

### In-App
- `ase48.png` - Displayed on the Home View welcome screen
- `ase64.png` - Used as the window icon at runtime

## Adding or Modifying Icons

1. Create PNG files at all four sizes (16, 32, 48, 64)
2. For Windows, bundle the PNGs into an ICO file using a tool like ImageMagick:
   ```bash
   convert icon16.png icon32.png icon48.png icon64.png icon.ico
   ```
3. Ensure icons use transparency and follow pixel-art style for clarity at small sizes
4. The 64px PNG is the source for macOS icon generation (scaled up automatically)
