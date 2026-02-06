# Document File Operations

*Last updated: 2025-12-30*

This directory contains file I/O operations for palette file formats. These utilities allow LibreSprite to load and save color palettes in various industry-standard formats.

## Supported Palette Formats

### COL Files (`.col`)
- **Format**: Raw binary palette format
- **Functions**: `load_col_file()`, `save_col_file()`
- **Description**: Simple binary format storing RGB color values

### GPL Files (`.gpl`)
- **Format**: GIMP Palette format
- **Functions**: `load_gpl_file()`, `save_gpl_file()`
- **Description**: Text-based palette format used by GIMP and other open-source graphics applications

### PAL Files (`.pal`)
- **Format**: Microsoft RIFF palette format
- **Functions**: `load_pal_file()`, `save_pal_file()`
- **Description**: Standard Windows palette format, commonly used for indexed color images

## Files

| File | Description |
|------|-------------|
| `col_file.cpp/h` | COL palette format I/O |
| `gpl_file.cpp/h` | GIMP palette format I/O |
| `pal_file.cpp/h` | Microsoft PAL format I/O |

## Usage

All load functions return a `std::shared_ptr<Palette>` containing the loaded colors, or `nullptr` on failure. Save functions return `true` on success.

```cpp
#include "doc/file/gpl_file.h"

// Load a GIMP palette
auto palette = doc::file::load_gpl_file("colors.gpl");

// Save a palette
doc::file::save_gpl_file(*palette, "output.gpl");
```
