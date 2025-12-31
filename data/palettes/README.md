# Palettes

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

This directory contains color palette files used by LibreSprite. Palettes define a fixed set of colors for indexed-color sprites and provide convenient color sets for any artwork.

## What Are Palettes?

A palette is a collection of colors that can be used when creating or editing sprites. In indexed color mode, each pixel references a color from the palette rather than storing RGB values directly. This is essential for:

- Retro/pixel art with limited color counts
- Matching historical hardware limitations (NES, Game Boy, etc.)
- Maintaining color consistency across artwork
- Reducing file sizes for indexed images

## Supported Formats

| Format | Extension | Description |
|--------|-----------|-------------|
| GIMP Palette | `.gpl` | Text-based format with RGB values and optional color names |
| PAL | `.pal` | Microsoft RIFF palette format |
| COL | `.col` | Raw color data format |

All palettes in this directory use the GPL format, which is human-readable and widely supported.

### GPL Format Example

```
GIMP Palette
#
# Optional comments/credits
#
  0   0   0	black
255 255 255	white
```

Each color line contains space-separated R, G, B values (0-255) followed by an optional tab and color name.

## Included Palettes

### Retro Hardware

| Palette | Colors | Description |
|---------|--------|-------------|
| `gameboy.gpl` | 4 | Original Game Boy green shades |
| `gameboy-color-type1.gpl` | 32 | Game Boy Color palette |
| `nes.gpl` | 56 | Nintendo Entertainment System |
| `nes-ntsc.gpl` | 56 | NES NTSC color variant |
| `master-system.gpl` | 32 | Sega Master System |
| `commodore64.gpl` | 16 | Commodore 64 |
| `commodore-vic20.gpl` | 16 | Commodore VIC-20 |
| `commodore-plus4.gpl` | 128 | Commodore Plus/4 |
| `atari2600-ntsc.gpl` | 256 | Atari 2600 NTSC |
| `atari2600-pal.gpl` | 128 | Atari 2600 PAL |
| `cga.gpl` | 16 | CGA 16-color mode |
| `vga-13h.gpl` | 256 | VGA Mode 13h default |
| `apple-ii.gpl` | 16 | Apple II |
| `zx-spectrum.gpl` | 16 | ZX Spectrum |
| `pico-8.gpl` | 16 | PICO-8 fantasy console |

### Pixel Art Palettes

| Palette | Colors | Author |
|---------|--------|--------|
| `db16.gpl` | 16 | DawnBringer 16 |
| `db32.gpl` | 32 | DawnBringer 32 (default palette) |
| `arne16.gpl` | 16 | Arne Niklas Jansson |
| `arne32.gpl` | 32 | Arne Niklas Jansson |
| `arne-paldac.gpl` | 256 | Arne Paldac collection |
| `aap-16.gpl` | 16 | Adigun A. Polack |
| `aap-64.gpl` | 64 | Adigun A. Polack |
| `aap-splendor128.gpl` | 128 | Adigun A. Polack |
| `cg-arne.gpl` | 16 | CG Arne variant |
| `eroge-copper.gpl` | 32 | Eroge Copper |
| `copper-tech.gpl` | 16 | Copper Tech |

### General Purpose

| Palette | Colors | Description |
|---------|--------|-------------|
| `web-safe-colors.gpl` | 216 | Web-safe color palette |
| `x11.gpl` | 140 | X11 named colors |
| `google-ui.gpl` | 256 | Google Material Design colors |
| `win16.gpl` | 16 | Windows 16-color palette |
| `solarized.gpl` | 16 | Solarized color scheme |
| `monokai.gpl` | 8 | Monokai color scheme |

## Adding Custom Palettes

### Method 1: User Directory

Place palette files in your user palettes directory:

- **Linux/macOS**: `~/.config/libresprite/palettes/`
- **Windows**: `%APPDATA%\LibreSprite\palettes\`

### Method 2: Application Data

Place palette files in this directory (`data/palettes/`) for bundling with LibreSprite.

### Creating a New GPL Palette

1. Create a text file with `.gpl` extension
2. Start with `GIMP Palette` header
3. Add optional comments prefixed with `#`
4. Add colors as `R G B` with optional tab-separated name

```
GIMP Palette
#
# My Custom Palette
# by Author Name
#
  0   0   0	black
 64  64  64	dark_gray
128 128 128	gray
255 255 255	white
```

## Related Code

| File | Purpose |
|------|---------|
| `src/app/file/palette_file.cpp` | Loads/saves palette files |
| `src/doc/file/gpl_file.h` | GPL format parser |
| `src/doc/file/pal_file.h` | PAL format parser |
| `src/doc/file/col_file.h` | COL format parser |
| `src/app/res/palettes_loader_delegate.cpp` | Palette resource discovery |
| `src/app/modules/palettes.cpp` | Default palette management |

The default palette (db32.gpl) is loaded at startup and used for new documents.

---

![Repobeats analytics](https://repobeats.axiom.co/api/embed/a1b2c3d4e5f6.svg "Repobeats analytics image")
