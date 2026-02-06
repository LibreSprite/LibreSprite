# Fonts

This directory contains font files used for rendering text throughout the LibreSprite user interface.

## Purpose

Fonts in LibreSprite are used for:

- **UI text** - menus, labels, buttons, dialogs
- **Editor overlays** - palette indices, frame numbers, layer names
- **Paste Text command** - inserting text into sprites

## Available Fonts

### TrueType Fonts (.ttf)

| File | Purpose |
|------|---------|
| `font.ttf` | Default UI font (Source Sans Pro) |
| `font-ar.ttf` | Arabic language support |
| `font-jp.ttf` | Japanese language support |
| `font-kr.ttf` | Korean language support |
| `font-zh.ttf` | Chinese language support |

Language-specific fonts are loaded automatically when the corresponding language is selected in preferences.

### Legacy Bitmap Fonts (.pcx)

| File | Purpose |
|------|---------|
| `allegro.pcx` | Legacy Allegro library font |
| `ase1.pcx` | Original Aseprite font (small) |
| `ase2.pcx` | Original Aseprite font (large) |
| `anipro_fixed.pcx` | Fixed-width font from Animation Pro |
| `anipro_variable.pcx` | Variable-width font from Animation Pro |

PCX fonts are sprite sheet fonts where each character is extracted from a grid layout.

## Supported Formats

- **TrueType (.ttf, .otf)** - Scalable vector fonts, rendered via FreeType library
- **PNG (.png)** - Sprite sheet fonts (character grid with key color separation)
- **PCX (.pcx)** - Legacy sprite sheet format from Allegro/DOS era

## Font Loading

Fonts are loaded by `SkinTheme::loadFonts()` in the following priority order:

1. User-specified font (from preferences)
2. Skin-specific language font (`skins/<skin>/font-<lang>.ttf`)
3. Global language font (`fonts/font-<lang>.ttf`)
4. Skin-specific default font (`skins/<skin>/font.ttf`)
5. Global default font (`fonts/font.ttf`)

Three font sizes are maintained:
- **Default font** - Standard UI text
- **Mini font** - Smaller text (palette indices, etc.)
- **Tiny font** - Smallest text

## License

The TrueType fonts (font.ttf and language variants) are Adobe Source Sans Pro, licensed under the SIL Open Font License 1.1. See `OFL.txt` for full license text.

The legacy PCX fonts are from the original Aseprite/Allegro projects.
