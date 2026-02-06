# Classic Skin

*Last updated: 2025-12-30*

The original LibreSprite/Aseprite theme by Ilija Melentijevic and David Capello. This is the legacy light-colored UI theme that was bundled with early versions of the software.

## Files

| File | Purpose |
|------|---------|
| `skin.xml` | Theme definition: dimensions, colors, cursor/tool/part coordinates, and CSS-like styles |
| `sheet.png` | Sprite atlas containing all UI graphics (buttons, icons, cursors, tools, etc.) |
| `palette.png` | Color palette used by the skin (indexed color reference) |

## How It Works

### skin.xml Structure

The XML file defines the theme in several sections:

- **dimensions** - UI element sizes (scrollbar width, tab height, etc.)
- **colors** - Named color values used throughout the UI (e.g., `text`, `face`, `selected`)
- **cursors** - Cursor sprites with coordinates in sheet.png and focus point offsets
- **tools** - Tool icons with coordinates in sheet.png
- **parts** - UI element graphics (buttons, checkboxes, sliders, etc.) with 9-slice coordinates
- **stylesheet** - CSS-like style definitions that combine colors and parts for widget states

### Mapping to sheet.png

Each element in skin.xml references a region in sheet.png using pixel coordinates:

```xml
<cursor id="normal" x="80" y="0" w="16" h="16" focusx="0" focusy="0" />
```

This defines the normal cursor as a 16x16 region starting at pixel (80, 0).

For scalable UI elements, 9-slice coordinates are used:

```xml
<part id="button_normal" x="48" y="0" w1="4" w2="6" w3="4" h1="4" h2="6" h3="6" />
```

The `w1/w2/w3` and `h1/h2/h3` values define the border and center regions for stretching.

## Differences from Default Skin

| Aspect | Classic | Default (Neutral Dark) |
|--------|---------|------------------------|
| Color scheme | Light (beige/cream tones) | Dark (gray/black tones) |
| Background | `#d3cbbe` (warm beige) | `#535353` (dark gray) |
| Text | `#000000` (black) | `#afafaf` (light gray) |
| Selected items | `#ff5555` (red) | `#FFFFFF` (white) |
| Window titlebar | `#7c909f` (blue-gray) | `#212121` (near-black) |
| Workspace | `#7d929e` (slate blue) | `#1f1f1f` (dark gray) |

Both skins share the same sprite sheet layout and structure, differing primarily in color definitions.

## Customization

To create a variant of this skin:

1. Copy this folder to a new name in the skins directory
2. Edit `skin.xml` to change colors, dimensions, or styles
3. Optionally edit `sheet.png` to modify icons and graphics
4. Select the new theme in Edit > Preferences > Theme
