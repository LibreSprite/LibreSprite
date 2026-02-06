# Desktop Integration

[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-blue)](https://libresprite.github.io)
[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

This directory contains desktop integration files for LibreSprite, enabling native integration with Linux and macOS desktop environments.

## Purpose

These files allow LibreSprite to:
- Appear in application menus and launchers
- Handle file associations for sprite and image formats
- Display thumbnails for supported file types in file managers
- Integrate with software centers (GNOME Software, KDE Discover, Flatpak)
- Register as the default application for Aseprite (.ase/.aseprite) files

## Key Files

| File | Platform | Purpose |
|------|----------|---------|
| `Info.plist` | macOS | Application bundle metadata (version, identifier, document types, icon) |
| `libresprite.desktop` | Linux | Freedesktop.org desktop entry for app launchers and menus |
| `io.github.libresprite.libresprite.metainfo.xml` | Linux | AppStream metadata for software centers (description, screenshots, releases) |
| `libresprite-thumbnailer` | Linux | Shell script that generates thumbnails using LibreSprite in batch mode |
| `screenshot.png` | All | Screenshot used in AppStream metadata for software center listings |
| `CMakeLists.txt` | Linux | CMake installation rules for desktop files |

## Subdirectories

### `gnome/`

GNOME/GTK thumbnailer configuration.

- `libresprite.thumbnailer` - Registers `libresprite-thumbnailer` script with GNOME's thumbnail system

### `kde/`

KDE/Qt thumbnailer plugin (optional, requires `WITH_QT_THUMBNAILER` CMake option).

- `aseprite_thumb_creator.cpp/h` - KIO ThumbCreator plugin implementation
- `librespritethumbnail.desktop` - KDE service registration
- `CMakeLists.txt` - Build configuration (requires KF5)

### `mime/`

MIME type definitions for the Aseprite file format.

- `aseprite.xml` - Registers `image/x-aseprite` MIME type with magic bytes and file extensions (`.ase`, `.aseprite`)

### `icons/`

Application icons in the Freedesktop.org hicolor icon theme structure.

```
icons/hicolor/
  16x16/apps/libresprite.png
  32x32/apps/libresprite.png
  48x48/apps/libresprite.png
  64x64/apps/libresprite.png
  256x256/apps/libresprite.png
```

## Desktop Environment Integration

### Linux Installation

The `CMakeLists.txt` installs files to standard XDG locations:

| File Type | Installation Path |
|-----------|-------------------|
| Desktop entry | `share/applications/` |
| Icons | `share/icons/` |
| AppStream metadata | `share/metainfo/` |
| MIME types | `share/mime/packages/` |
| Thumbnailer | `share/thumbnailers/` |

After installation, update system caches:

```bash
# Update desktop database
update-desktop-database ~/.local/share/applications

# Update MIME database
update-mime-database ~/.local/share/mime

# Update icon cache
gtk-update-icon-cache ~/.local/share/icons/hicolor
```

### macOS Integration

The `Info.plist` is used when creating the `.app` bundle. It defines:
- Bundle identifier: `libresprite.libresprite.github`
- Supported document types (JPEG, PNG) for drag-and-drop
- Application icon reference

## Updating Desktop Integration Files

### Adding New File Associations

1. **Linux**: Add MIME types to:
   - `libresprite.desktop` (MimeType field)
   - `gnome/libresprite.thumbnailer` (MimeType field)
   - `kde/librespritethumbnail.desktop` (MimeType field)

2. **macOS**: Add new `CFBundleDocumentTypes` entries to `Info.plist`

### Updating Application Metadata

- **Version**: Update `Info.plist` (CFBundleVersion) and `metainfo.xml` (releases)
- **Description**: Edit `metainfo.xml` summary and description
- **Screenshot**: Replace `screenshot.png` and update URL in `metainfo.xml`

### Adding Icon Sizes

Add new PNG files to `icons/hicolor/<size>/apps/libresprite.png`. Common sizes: 16, 22, 24, 32, 48, 64, 128, 256, 512.

## Supported MIME Types

The following MIME types are registered for file associations and thumbnailing:

- `image/bmp` - BMP images
- `image/gif` - GIF animations
- `image/jpeg` - JPEG images
- `image/png` - PNG images
- `image/x-pcx` - PCX images
- `image/x-tga` - TGA images
- `image/vnd.microsoft.icon` - ICO files
- `video/x-flic` - FLIC animations
- `image/webp` - WebP images
- `image/x-aseprite` - Aseprite sprite files (.ase, .aseprite)

![Repobeats analytics](https://repobeats.axiom.co/api/embed/e1c3f3c8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8.svg "Repobeats analytics image")
