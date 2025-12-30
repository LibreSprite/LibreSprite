# UI Theming/Skinning

*Last updated: 2025-12-30*

This directory contains the theming system that controls the visual appearance of LibreSprite's user interface.

## Overview

LibreSprite uses a skinnable UI system where visual elements (colors, fonts, widget appearances) are defined in external XML files and sprite sheets. This allows for customizable themes without modifying code.

## Key Classes

### Theme Core

- **SkinTheme** (`skin_theme.h/.cpp`) - The main theme implementation. Extends `ui::Theme` to provide:
  - Paint methods for all standard widgets (buttons, checkboxes, sliders, etc.)
  - Font loading and management (default, mini, tiny fonts)
  - Cursor definitions
  - Color and dimension lookups by ID
  - Widget initialization with theme-specific settings

### Style System

- **Style** (`style.h/.cpp`) - Represents a named style that can be applied to widgets. Styles define visual properties like colors, borders, and padding.

- **StyleSheet** (`style_sheet.h/.cpp`) - Collection of styles loaded from XML. Provides style lookup by ID.

### Skin Parts

- **SkinPart** (`skin_part.h/.cpp`) - Represents a portion of the skin sprite sheet. Used for 9-slice scaling of UI elements (borders, backgrounds).

### Widget Properties

- **SkinProperty** (`skin_property.h/.cpp`) - Attachable property for widgets to store skin-specific data

- **SkinSliderProperty** (`skin_slider_property.h/.cpp`) - Slider-specific skin properties

- **SkinStyleProperty** (`skin_style_property.h/.cpp`) - Style-specific skin properties

### Button Customization

- **SkinButton** (`skin_button.h`) - Extended button with skin-aware rendering

- **ButtonIconImpl** (`button_icon_impl.h/.cpp`) - Implementation of button icons using skin parts

### Utilities

- **BackgroundRepeat** (`background_repeat.h`) - Enum for background image repeat modes

## Skin File Structure

Skins are loaded from `data/skins/` and consist of:

```
data/skins/<skin-name>/
    sheet.png          - Sprite sheet with UI elements
    skin.xml           - Defines parts, colors, dimensions
    theme.xml          - Defines styles and widget theming
    fonts/             - Theme-specific fonts
```

## How Theming Works

1. `SkinTheme::onRegenerate()` loads the skin XML and sprite sheet
2. Parts are sliced from the sheet and stored by ID
3. Colors and dimensions are parsed from XML
4. Styles are loaded into the `StyleSheet`
5. Each widget's `paint*()` method uses these resources to render

## Paint Methods

`SkinTheme` provides paint methods for every widget type:

- `paintButton()`, `paintCheckBox()`, `paintRadioButton()`
- `paintEntry()`, `paintSlider()`, `paintComboBoxEntry()`
- `paintListBox()`, `paintListItem()`, `paintMenu()`, `paintMenuItem()`
- `paintWindow()`, `paintPopupWindow()`, `paintTooltip()`
- `paintView()`, `paintViewScrollbar()`, `paintSplitter()`
- And more...

## Usage

```cpp
// Get a skin part for custom drawing
SkinPartPtr part = SkinTheme::instance()->getPartById("button_normal");

// Get a color by ID
gfx::Color bgColor = get_color_by_id("button_face");

// Get a dimension by ID (auto-scaled for GUI scale)
int margin = SkinTheme::instance()->getDimensionById("button_margin");
```
