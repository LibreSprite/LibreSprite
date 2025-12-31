# Skins

This directory contains UI themes (skins) for LibreSprite. Skins control the visual appearance of the application, including colors, widget graphics, cursors, tool icons, and CSS-like styling.

## What Are Skins?

A skin is a collection of assets that define LibreSprite's visual appearance:

- **Colors** for text, backgrounds, buttons, windows, timeline, etc.
- **UI Graphics** for buttons, scrollbars, tabs, tooltips, and other widgets
- **Cursors** for different tool and interaction states
- **Tool Icons** for the toolbar
- **Dimensions** like scrollbar size, tab height, and timeline borders
- **Stylesheets** providing CSS-like styling rules for widgets

## Skin Structure

Each skin is a subdirectory containing three required files:

| File | Description |
|------|-------------|
| `skin.xml` | Defines colors, dimensions, cursor/tool/part coordinates, and stylesheets |
| `sheet.png` | Sprite sheet containing all UI graphics (buttons, icons, cursors, etc.) |
| `palette.png` | 16x16 color palette used by the skin |

### skin.xml Sections

The `skin.xml` file contains these main sections:

```xml
<skin name="Theme Name" author="Author" url="...">
  <dimensions>   <!-- Widget sizes: scrollbar, tabs, timeline, etc. -->
  <colors>       <!-- Named colors referenced by styles -->
  <cursors>      <!-- Cursor sprite locations and focus points -->
  <tools>        <!-- Tool icon locations in sheet.png -->
  <parts>        <!-- UI element locations (buttons, tabs, scrollbars, etc.) -->
  <stylesheet>   <!-- CSS-like styling rules for widgets -->
</skin>
```

### Part Definitions

Parts reference rectangular regions in `sheet.png`. Simple parts use x/y/w/h:

```xml
<part id="check_normal" x="48" y="64" w="8" h="8" />
```

9-slice parts define stretchable regions with w1/w2/w3 and h1/h2/h3:

```xml
<part id="button_normal" x="48" y="0" w1="4" w2="6" w3="4" h1="4" h2="6" h3="6" />
```

## Available Skins

| Skin | Description |
|------|-------------|
| **default** | "Neutral Dark" theme by BlackRaven - Modern dark UI with neutral gray tones |
| **classic** | Original Aseprite-style light theme by Ilija Melentijevic and David Capello |

## Changing Themes

1. Open LibreSprite
2. Go to **Edit > Preferences > Theme**
3. Select a theme from the list
4. Click **Select**, then **OK**
5. Restart LibreSprite

## Creating a Custom Skin

1. Copy an existing skin directory (e.g., `default/`) to a new folder
2. Rename the folder to your theme name
3. Edit `skin.xml`:
   - Update the `name`, `author`, and `url` attributes
   - Modify colors in the `<colors>` section
   - Adjust dimensions if needed
4. Edit `sheet.png`:
   - Maintain the same layout and coordinates
   - Modify graphics while keeping element positions unchanged
5. Edit `palette.png` if using custom colors
6. Place your skin in one of these locations:
   - **Windows**: `%APPDATA%\Libresprite\data\skins\` or `<install-dir>\data\skins\`
   - **Linux**: `~/.config/libresprite/data/skins/`
   - **macOS**: `~/Library/Application Support/LibreSprite/data/skins/`

### Tips for Creating Skins

- Study the existing `skin.xml` files to understand the color and part structure
- Use an indexed color image editor to maintain palette consistency
- Test your skin with different dialogs and tools to ensure complete coverage
- Keep `sheet.png` dimensions consistent (300x272 for the bundled skins)

## See Also

- [Theming.md](/Theming.md) - User guide for finding and installing community themes
- `src/app/ui/skin/` - Source code for skin loading and rendering
