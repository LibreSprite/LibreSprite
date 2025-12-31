# Widget Files

This directory contains XML-based dialog and UI layout definitions for LibreSprite. These files declaratively describe the structure, appearance, and behavior of dialogs, popups, and application windows.

## Overview

Widget files are loaded at runtime by the `WidgetLoader` class (`src/app/widget_loader.cpp`) which parses the XML and constructs the corresponding C++ UI widget hierarchy. This separation allows UI layouts to be modified without recompiling the application.

### How Widget Files Work

1. Each file contains a `<gui>` root element with one or more widget definitions
2. Widgets are identified by their `id` attribute
3. C++ code loads widgets using the `load_widget<T>()` template function from `src/app/load_widget.h`
4. The loader recursively builds the widget tree, connecting child widgets to parents

**Example usage in C++ code:**

```cpp
#include "app/load_widget.h"

auto* window = app::load_widget<Window>("new_sprite.xml", "new_sprite");
```

## XML Structure

### Root Element

All widget files must have a `<gui>` root element:

```xml
<gui>
  <window id="my_dialog" text="Dialog Title">
    <!-- child widgets -->
  </window>
</gui>
```

### Common Widget Elements

| Element | Description | Key Attributes |
|---------|-------------|----------------|
| `window` | Top-level dialog/window | `text`, `id`, `desktop`, `noborders` |
| `vbox` | Vertical box container | - |
| `hbox` | Horizontal box container | - |
| `box` | Generic box | `horizontal`, `vertical` |
| `grid` | Grid layout | `columns`, `same_width_columns` |
| `panel` | Panel container (for tabbed interfaces) | - |
| `label` | Static text | `text`, `center`, `right` |
| `button` | Push button | `text`, `icon`, `closewindow`, `bevel` |
| `check` | Checkbox | `text`, `looklike` |
| `radio` | Radio button | `group`, `looklike` |
| `entry` | Text input field | `maxsize`, `suffix`, `readonly` |
| `intentry` | Integer input | `min`, `max`, `suffix` |
| `combobox` | Dropdown selector | `editable` |
| `listbox` | List of items | - |
| `listitem` | Item in listbox/combobox | `text`, `value` |
| `slider` | Value slider | `min`, `max` |
| `separator` | Visual separator/section header | `text`, `horizontal`, `vertical` |
| `splitter` | Resizable split pane | `horizontal`, `vertical`, `by`, `position` |
| `view` | Scrollable viewport | `maxsize` |
| `link` | Clickable hyperlink | `text`, `url` |
| `image` | Static image | `file` |
| `buttonset` | Grouped toggle buttons | `columns`, `multiple` |
| `item` | Item in buttonset | `icon`, `text`, `hspan`, `vspan` |
| `search` | Search input field | - |
| `colorpicker` | Color selector button | - |
| `boxfiller` | Flexible spacer | - |

### Common Attributes

| Attribute | Description | Applicable To |
|-----------|-------------|---------------|
| `id` | Unique identifier for C++ access | All widgets |
| `text` | Display text (supports `&` for mnemonics) | Most widgets |
| `tooltip` | Hover tooltip (supports `&#10;` for newlines) | All widgets |
| `tooltip_dir` | Tooltip direction (`top`, `bottom`, `left`, `right`) | All widgets |
| `expansive` | Fill available space | All widgets |
| `homogeneous` | Equal-size children | Box containers |
| `magnet` | Receives initial focus | Interactive widgets |
| `disabled` | Disabled state | Interactive widgets |
| `selected` | Pre-selected state | Toggleable widgets |
| `noborders` | Remove padding/borders | Containers |
| `width`, `height` | Fixed dimensions (in pixels) | All widgets |
| `minwidth`, `minheight` | Minimum dimensions | All widgets |
| `maxwidth`, `maxheight` | Maximum dimensions | All widgets |
| `border` | Border size | Containers |
| `childspacing` | Space between children | Containers |
| `style` | Skin style reference | All widgets |

### Grid-Specific Attributes

When placing widgets inside a `<grid>`:

| Attribute | Description |
|-----------|-------------|
| `cell_hspan` | Horizontal span (columns) |
| `cell_vspan` | Vertical span (rows) |
| `cell_align` | Alignment: `horizontal`, `vertical`, `left`, `center`, `right`, `top`, `middle`, `bottom` |

### Alignment Attributes

| Attribute | Values |
|-----------|--------|
| `horizontal` / `vertical` | Boolean for orientation |
| `left` / `center` / `right` | Horizontal alignment |
| `top` / `middle` / `bottom` | Vertical alignment |

## File Reference

| File | Purpose |
|------|---------|
| `main_window.xml` | Application main window layout with placeholder regions |
| `home_view.xml` | Welcome screen with recent files/folders |
| `options.xml` | Preferences dialog with tabbed sections |
| `keyboard_shortcuts.xml` | Keyboard shortcut configuration |
| `new_sprite.xml` | New sprite creation dialog |
| `canvas_size.xml` | Canvas resize dialog |
| `sprite_size.xml` | Sprite resize dialog |
| `sprite_properties.xml` | Sprite metadata dialog |
| `layer_properties.xml` | Layer settings dialog |
| `frame_properties.xml` | Frame duration settings |
| `cel_properties.xml` | Cel opacity settings |
| `new_layer.xml` | New layer/layer set dialogs |
| `export_sprite_sheet.xml` | Sprite sheet export options |
| `import_sprite_sheet.xml` | Sprite sheet import options |
| `file_selector.xml` | File open/save dialog |
| `gif_options.xml` | GIF export settings |
| `jpeg_options.xml` | JPEG export settings |
| `webp_options.xml` | WebP export settings |
| `palette_popup.xml` | Palette selection popup |
| `palette_size.xml` | Resize palette dialog |
| `goto_frame.xml` | Jump to frame dialog |
| `grid_settings.xml` | Grid configuration |
| `timeline_conf.xml` | Timeline settings |
| `undo_history.xml` | Undo history panel |
| `replace_color.xml` | Color replacement filter |
| `convolution_matrix.xml` | Convolution matrix filter |
| `despeckle.xml` | Despeckle filter settings |
| `modify_selection.xml` | Selection modification options |
| `select_accelerator.xml` | Keyboard shortcut picker |
| `send_crash.xml` | Crash report dialog |
| `user_data.xml` | User data editor |
| `advanced_mode.xml` | Advanced mode settings |
| `brush_slot_params.xml` | Brush slot parameters |
| `font_popup.xml` | Font selector popup |

## Creating a New Dialog

1. Create a new XML file in `data/widgets/`:

```xml
<!-- LibreSprite -->
<!-- Copyright (C) 2024 LibreSprite contributors -->
<gui>
  <window id="my_dialog" text="My Dialog">
    <vbox>
      <grid columns="2">
        <label text="Input:" />
        <entry id="my_input" maxsize="64" magnet="true" />
      </grid>

      <separator horizontal="true" />

      <hbox>
        <boxfiller />
        <hbox homogeneous="true">
          <button text="&amp;OK" id="ok" closewindow="true" magnet="true" width="60" />
          <button text="&amp;Cancel" closewindow="true" />
        </hbox>
      </hbox>
    </vbox>
  </window>
</gui>
```

2. Load and use it in C++ code:

```cpp
#include "app/load_widget.h"
#include "ui/window.h"
#include "ui/entry.h"

void showMyDialog() {
    auto window = app::load_widget<ui::Window>("my_dialog.xml", "my_dialog");
    auto input = window->findChildT<ui::Entry>("my_input");

    window->openWindowInForeground();

    if (window->closer() == window->findChild("ok")) {
        std::string value = input->text();
        // Process the input...
    }
}
```

## Common Patterns

### Dialog Footer with OK/Cancel

```xml
<hbox>
  <boxfiller />
  <hbox homogeneous="true">
    <button text="&amp;OK" id="ok" closewindow="true" magnet="true" width="60" />
    <button text="&amp;Cancel" closewindow="true" />
  </hbox>
</hbox>
```

### Labeled Input Field

```xml
<grid columns="2">
  <label text="Name:" />
  <entry id="name" maxsize="256" cell_align="horizontal" />
</grid>
```

### Section with Header

```xml
<separator text="Section Title" left="true" horizontal="true" />
<vbox>
  <!-- section content -->
</vbox>
```

### Toggle Button Group

```xml
<buttonset columns="3" id="my_options">
  <item text="Option A" icon="icon_a" />
  <item text="Option B" icon="icon_b" />
  <item text="Option C" icon="icon_c" />
</buttonset>
```

### Dropdown with Predefined Values

```xml
<combobox id="scale">
  <listitem text="100%" value="1" />
  <listitem text="200%" value="2" />
  <listitem text="300%" value="3" />
</combobox>
```

## Related Source Code

| File | Description |
|------|-------------|
| `src/app/widget_loader.cpp` | XML parser and widget factory |
| `src/app/widget_loader.h` | WidgetLoader class definition |
| `src/app/load_widget.h` | Template helper function |
| `src/ui/` | Base widget library (Button, Entry, etc.) |
| `src/app/ui/` | Application-specific widgets |
| `src/app/ui/skin/` | Theme/skin system |
