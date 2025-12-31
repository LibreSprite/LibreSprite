# LibreSprite Runtime Assets

[![LibreSprite](https://img.shields.io/badge/LibreSprite-Runtime%20Data-blue)](https://github.com/LibreSprite/LibreSprite)

This directory contains all runtime assets required by LibreSprite. These files are loaded at application startup and during operation to provide UI definitions, themes, translations, color palettes, and other resources.

## Overview

The `data/` directory is bundled with LibreSprite distributions and must be present at runtime. The application locates these files relative to its executable path.

## Key Files

| File | Purpose |
|------|---------|
| `gui.xml` | Main application configuration: keyboard shortcuts, menu structure, toolbar layout, and command bindings |
| `pref.xml` | Preference definitions with types, defaults, and validation. Code-generated into typed C++ accessors at build time |
| `convmatr.def` | Convolution matrix definitions for image filters (blur, sharpen, edge detect, emboss, etc.) |
| `splash.ase` | Splash screen artwork in native ASE format |
| `splash.png` | Splash screen artwork in PNG format (fallback) |

### gui.xml

Defines the complete keyboard shortcut system with platform-specific bindings (`shortcut`, `mac`, `win`, `linux`), menu hierarchy, and tool configurations. Example:

```xml
<key command="NewFile" shortcut="Ctrl+N" mac="Cmd+N" />
```

### pref.xml

Declarative preference schema with enums and typed options. Used by the build system to generate `src/app/pref/preferences.h`. Example:

```xml
<option id="language" type="std::string" default="&quot;en&quot;" />
```

### convmatr.def

Custom text format defining convolution matrices with kernel dimensions, data, divisor, bias, and target channels. Parsed by `src/effect/convmatr.c`.

## Subdirectories

| Directory | Contents | Description |
|-----------|----------|-------------|
| `fonts/` | TTF, PCX | UI fonts including language-specific fonts (Arabic, Japanese, Korean, Chinese) |
| `icons/` | ICO, PNG | Application and document icons at multiple resolutions (16, 32, 48, 64px) |
| `languages/` | JSON | Translation strings for 14 languages (en, de, es, fr, it, jp, kr, zh, etc.) |
| `palettes/` | GPL | 38 color palettes including retro console palettes (NES, Game Boy, C64, etc.) |
| `scripts/` | JS | Bundled JavaScript scripts for automation and effects |
| `skins/` | PNG, XML | UI themes with sprite sheets and style definitions |
| `widgets/` | XML | Dialog and panel layout definitions (40 widget files) |

## File Formats

### Palettes (GPL)

GIMP Palette format with RGB values and optional color names:

```
GIMP Palette
# Palette metadata
255 255 255    white
  0   0   0    black
```

### Language Files (JSON)

Key-value pairs mapping UI string identifiers to localized text:

```json
{
  "New Sprite": "Nuevo Sprite",
  "Save File": "Guardar Archivo"
}
```

### Skin Definition (skin.xml)

UI theme configuration with dimensions, colors, and sprite sheet mappings:

```xml
<skin name="Neutral Dark" author="BlackRaven#7273">
  <dimensions>
    <dim id="scrollbar_size" value="12" />
  </dimensions>
  <colors>
    <color id="background" value="#3a3a3a" />
  </colors>
</skin>
```

### Widget Layouts (XML)

Dialog structure definitions using LibreSprite's XML-based UI system:

```xml
<window text="New Sprite" id="new_sprite">
  <grid columns="2">
    <label text="Width:" />
    <entry id="width" maxsize="8" suffix="px" />
  </grid>
</window>
```

## Runtime Usage

1. **Application Startup**: Loads `gui.xml` for shortcuts and menus, `pref.xml` schema, and selected skin
2. **Language Selection**: Loads appropriate `languages/*.json` based on user preference
3. **Dialog Display**: Loads widget XML files on-demand when dialogs are opened
4. **Tool Use**: Accesses `convmatr.def` for convolution matrix filters
5. **Palette Operations**: Loads palette files from `palettes/` when user selects them

## Adding Custom Content

- **Palettes**: Add GPL files to `palettes/` - they appear automatically in the palette selector
- **Scripts**: Add JS files to `scripts/` - accessible via Script menu
- **Skins**: Create a new directory in `skins/` with `skin.xml`, `sheet.png`, and `palette.png`

---

![Repobeats analytics](https://repobeats.axiom.co/api/embed/a7c0f6e4f6b3c8d9e0f1a2b3c4d5e6f7a8b9c0d1.svg "Repobeats analytics image")
