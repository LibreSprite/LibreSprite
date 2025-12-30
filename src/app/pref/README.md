# Preferences

*Last updated: 2025-12-30*

User preferences and application settings system.

## Architecture

Preferences are defined in `data/pref.xml` and code-generated into typed C++ accessors. The `Preferences` class provides access to all settings.

## Key Classes

| Class | Purpose |
|-------|---------|
| `Preferences` | Main preferences container with typed accessors |
| `Option<T>` | Template for individual preference values |
| `OptionIO` | Serialization helpers for preference types |

## Files

| File | Purpose |
|------|---------|
| `preferences.cpp/h` | Main Preferences class implementation |
| `option.h` | Option template for typed preference storage |
| `option_io.h` | Read/write preferences to/from config files |

## Usage

```cpp
// Access preferences
auto& prefs = App::instance()->preferences();

// Read a preference
bool showGrid = prefs.document().grid.visible();

// Modify a preference
prefs.document().grid.visible(true);
```

## Adding New Preferences

1. Add entry in `data/pref.xml`
2. Run code generation (handled by build)
3. Access via `Preferences` class
