# Windows Installer

[![Inno Setup](https://img.shields.io/badge/Inno%20Setup-6.x-blue)](https://jrsoftware.org/isinfo.php)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)](https://github.com/LibreSprite/LibreSprite)

This directory contains the Windows installer configuration for LibreSprite.

## Overview

The installer is built using [Inno Setup](https://jrsoftware.org/isinfo.php), a free and open-source script-driven installation system for Windows. Inno Setup creates professional Windows installers with features like:

- Start Menu and Desktop shortcuts
- File type associations
- Uninstaller generation
- License agreement display
- Modern wizard-style interface

## Prerequisites

1. **Inno Setup 6.x** - Download from [jrsoftware.org](https://jrsoftware.org/isdl.php)
2. **Built LibreSprite** - The application must be compiled first (see main README)
3. **Windows environment** - Inno Setup only runs on Windows

## Building the Installer

1. Build LibreSprite for Windows (output goes to `build/bin/`)
2. Open `WindowsInstaller.iss` in Inno Setup Compiler
3. Press F9 or select Build > Compile
4. The installer will be created as `LibreSpriteInstaller.exe` in this directory

**Command line build:**
```batch
iscc WindowsInstaller.iss
```

## Configuration Summary

| Setting | Value |
|---------|-------|
| Output filename | `LibreSpriteInstaller.exe` |
| Default install path | `%ProgramFiles%\LibreSprite` |
| File association | `.ase` files |
| Compression | LZMA (solid) |
| License | `LICENSE.txt` from repo root |

### Included Files

The installer packages:
- `libresprite.exe` - Main application
- `gen.exe` - Code generation tool
- `data/*` - Runtime assets (skins, palettes, scripts, etc.)

### Registry Entries

The installer registers LibreSprite as a handler for `.ase` files, enabling:
- Double-click to open `.ase` files in LibreSprite
- "Open with" menu integration
- File type icon association

## Customization

Edit `WindowsInstaller.iss` to modify:
- **Version**: Update `#define MyAppVersion`
- **Install location**: Modify `DefaultDirName`
- **File associations**: Edit `MyAppAssocExt` and registry section
- **Included files**: Update the `[Files]` section

## References

- [Inno Setup Documentation](https://jrsoftware.org/ishelp/)
- [Inno Setup FAQ](https://jrsoftware.org/isfaq.php)

---

![Repobeats analytics](https://repobeats.axiom.co/api/embed/c1a6f0d0e4b8c9d2e3f4a5b6c7d8e9f0a1b2c3d4.svg "Repobeats analytics image")
