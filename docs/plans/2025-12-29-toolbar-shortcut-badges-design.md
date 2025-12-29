# Toolbar Shortcut Badges Design

## Overview

Add visual keyboard shortcut badges to toolbar tool icons, showing the shortcut key in the bottom-right corner of each tool button.

## Visual Design

- **Position:** Bottom-right corner of each tool button
- **Shape:** Small rounded rectangle (pill/badge)
- **Background:** Semi-transparent dark (black at ~80% opacity)
- **Text:** White letter, small font (~8-9px)
- **Content:** Single character for simple shortcuts (B, E, M); just the letter for compound shortcuts (Shift+M shows "M")

## Edge Cases

- Tools without shortcuts: No badge shown
- Multi-key shortcuts (Shift+M): Show just the letter
- Quicktools (Alt, Space, Ctrl): Not shown as badges

## Implementation

### Files to Modify

1. `data/pref.xml` - Add preference definition
2. `data/widgets/options.xml` - Add checkbox to General section
3. `src/app/ui/toolbar.cpp` - Add badge drawing in `onPaint()` and `ToolStrip::onPaint()`

### Preference

- Section: `general`
- Option: `show_tool_shortcuts`
- Type: `bool`
- Default: `true`

### Badge Drawing Logic

In `ToolBar::onPaint()` after drawing the tool icon:

1. Check preference is enabled
2. Get keyboard shortcut via `KeyboardShortcuts::instance()->tool(tool)`
3. Extract key letter from accelerator
4. Calculate badge position (bottom-right of toolrc)
5. Draw filled rounded rect with dark background
6. Draw white letter centered in badge

Same logic applies to `ToolStrip::onPaint()` for the popup tool strip.
