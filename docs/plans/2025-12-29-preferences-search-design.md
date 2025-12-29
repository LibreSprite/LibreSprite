# Preferences Search Feature Design

## Overview

Expand the command palette search bar to index individual preference settings. When a user searches for something like "grid color", it will show "Edit > Preferences > Grid & Background > Grid Color" and open the Preferences dialog directly to that section.

## Requirements

- Search for individual settings within dialogs (e.g., "grid color")
- Display full path: "Edit > Preferences > [Section] > [Setting]"
- Open Preferences dialog to the correct section when selected
- Setting-level granularity (not just section-level)

## Implementation

### 1. Modify OptionsCommand (cmd_options.cpp)

Add parameter support to open a specific section:

```cpp
void OptionsCommand::onLoadParams(const Params& params) {
  m_section = params.get("section");
}

void OptionsCommand::onExecute(Context* context) {
  int curSection = 0;
  if (!m_section.empty()) {
    curSection = sectionNameToIndex(m_section);
  }
  OptionsWindow window(context, curSection);
  // ...
}
```

Section name to index mapping:
- "general" → 0
- "editor" → 1
- "timeline" → 2
- "cursors" → 3
- "grid" → 4
- "undo" → 5
- "theme" → 6
- "experimental" → 7

### 2. Extend MenuSearch (menu_search.cpp)

Add preference entries alongside command entries:

```cpp
struct PreferenceEntry {
  std::string settingName;      // "Grid Color"
  std::string sectionName;      // "Grid & Background"
  std::string sectionId;        // "grid" (for Options command param)
  std::string displayName;      // "Edit > Preferences > Grid & Background > Grid Color"
  std::string searchText;       // lowercase keywords
};

std::vector<PreferenceEntry> m_allPreferences;
```

Add `buildPreferenceIndex()` method that creates entries for all settings.

### 3. Settings Index

| Section (Index) | Setting | Keywords |
|-----------------|---------|----------|
| General (0) | Language | language, locale |
| General (0) | Screen Scaling | screen, scaling, scale, display |
| General (0) | UI Elements Scaling | ui, elements, scaling |
| General (0) | GPU Acceleration | gpu, acceleration, hardware |
| General (0) | Expand Menubar on Mouseover | menubar, expand, mouseover |
| General (0) | Data Recovery | data, recovery, autosave, backup |
| General (0) | Show Full Path | path, filename, full |
| General (0) | Show Keyboard Shortcuts | keyboard, shortcuts, toolbar, icons |
| Editor (1) | Zoom with Scroll Wheel | zoom, scroll, wheel, mouse |
| Editor (1) | Zoom from Center | zoom, center |
| Editor (1) | Show Scrollbars | scrollbars, editor |
| Editor (1) | Right-click Behavior | right, click, behavior, mouse |
| Editor (1) | Auto Opaque | opaque, transparent, selection |
| Editor (1) | Keep Selection After Clear | selection, clear, keep |
| Timeline (2) | Show Timeline Automatically | timeline, auto, automatic |
| Timeline (2) | Rewind on Stop | rewind, stop, animation |
| Cursors (3) | Native Mouse Cursor | native, mouse, cursor, system |
| Cursors (3) | Cursor Color | cursor, color, precise |
| Cursors (3) | Brush Preview | brush, preview |
| Grid (4) | Grid Color | grid, color |
| Grid (4) | Grid Opacity | grid, opacity |
| Grid (4) | Pixel Grid Color | pixel, grid, color |
| Grid (4) | Pixel Grid Opacity | pixel, grid, opacity |
| Grid (4) | Checked Background Size | checked, background, size |
| Grid (4) | Checked Background Colors | checked, background, colors |
| Undo (5) | Undo Limit | undo, limit, memory |
| Undo (5) | Go to Modified Frame | undo, modified, frame, layer |
| Undo (5) | Non-linear History | undo, nonlinear, history |
| Theme (6) | Theme Selection | theme, skin, appearance |
| Experimental (7) | Native File Dialog | native, file, dialog |
| Experimental (7) | Flash Layer | flash, layer, highlight |

### 4. Execution Flow

When user selects a preference entry:
1. Get the `sectionId` from the entry (e.g., "grid")
2. Create params with `section=grid`
3. Execute the Options command with those params
4. OptionsWindow opens directly to section index 4

## Files Modified

1. `src/app/commands/cmd_options.cpp` - Add section parameter support
2. `src/app/ui/menu_search.h` - Add PreferenceEntry struct
3. `src/app/ui/menu_search.cpp` - Add preference indexing and execution
