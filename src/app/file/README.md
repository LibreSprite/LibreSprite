# File Formats

*Last updated: 2025-12-30*

File format loaders and savers for various image and animation formats.

## Architecture

Each format implements `FileFormat` interface with `onLoad()` and `onSave()` methods. Formats are registered with `FileFormatsManager`.

## Supported Formats

| File | Format | Extensions |
|------|--------|------------|
| `ase_format.cpp` | Aseprite/LibreSprite native | .ase, .aseprite |
| `png_format.cpp` | PNG | .png |
| `gif_format.cpp` | GIF (animated) | .gif |
| `jpeg_format.cpp` | JPEG | .jpg, .jpeg |
| `bmp_format.cpp` | Windows Bitmap | .bmp |
| `tga_format.cpp` | Targa | .tga |
| `pcx_format.cpp` | PCX | .pcx |
| `fli_format.cpp` | FLI/FLC animation | .fli, .flc |
| `ico_format.cpp` | Windows Icon | .ico |
| `webp_format.cpp` | WebP | .webp |
| `qoi_format.cpp` | Quite OK Image | .qoi |
| `pixly_format.cpp` | Pixly format | .pixly |
| `extension_format.cpp` | Extension packages | .aseprite-extension |

## Key Classes

| Class | Purpose |
|-------|---------|
| `FileFormat` | Base class for format implementations |
| `FileFormatsManager` | Registry of available formats |
| `FormatOptions` | Format-specific save options (e.g., GIF loop count) |

## Palette Files

`palette_file.cpp/h` handles loading palettes from various formats (GPL, PAL, etc.).
