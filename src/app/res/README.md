# Resources

*Last updated: 2025-12-30*

Resource loading system for palettes, brushes, and other assets.

## Architecture

Uses a delegate pattern for loading resources asynchronously. Resources are loaded from the data directory and user profile.

## Key Classes

| Class | Purpose |
|-------|---------|
| `ResourcesLoader` | Async loader for resources (palettes, brushes) |
| `ResourcesLoaderDelegate` | Interface for handling loaded resources |
| `PalettesLoaderDelegate` | Specialized loader for palette files |
| `Resource` | Base interface for loadable resources |

## Files

| File | Purpose |
|------|---------|
| `resources_loader.cpp/h` | Main resource loading logic |
| `resources_loader_delegate.h` | Delegate interface |
| `palettes_loader_delegate.cpp` | Palette-specific loading |
| `resource.h` | Resource base class |
| `http.h` | HTTP utilities for remote resources |

## Resource Locations

Resources are loaded from:
1. `data/` directory (bundled with app)
2. User profile directory (custom resources)
3. Remote URLs (extensions, community resources)
