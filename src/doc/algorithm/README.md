# Document Manipulation Algorithms

*Last updated: 2025-12-30*

This directory contains low-level image processing algorithms used for document manipulation in LibreSprite. These algorithms operate on the `doc::Image` class and provide core functionality for transforming, filling, and resizing sprite images.

## Key Algorithms

### Image Transformation
- **`flip_image`** - Flips images horizontally or vertically within a rectangular region or masked area
- **`rotate`** - Rotates images around a center point, includes `scale_image` and `parallelogram` transformation functions
- **`rotsprite`** - Implements the RotSprite algorithm for pixel-art-friendly rotation with reduced aliasing
- **`shift_image`** - Shifts image pixels with wrapping

### Resizing
- **`resize_image`** - Resizes images with multiple interpolation methods:
  - `RESIZE_METHOD_NEAREST_NEIGHBOR` - Fast, preserves hard edges
  - `RESIZE_METHOD_BILINEAR` - Smooth interpolation
  - `RESIZE_METHOD_ROTSPRITE` - Pixel-art optimized scaling
- Also includes `fixup_image_transparent_colors` for pre-processing transparent pixels

### Drawing & Fill
- **`floodfill`** - Flood fill algorithm with tolerance and contiguous fill options
- **`polygon`** - Polygon drawing and filling operations

### Utility
- **`shrink_bounds`** - Calculates the minimal bounding box of non-transparent pixels
- **`hline`** - Horizontal line drawing primitive (used by other algorithms)
- **`flip_type`** - Enumeration for flip direction types

## Files

| File | Description |
|------|-------------|
| `flip_image.cpp/h` | Horizontal/vertical image flipping |
| `flip_type.h` | FlipType enumeration |
| `floodfill.cpp/h` | Flood fill implementation |
| `hline.h` | Horizontal line callback type |
| `polygon.cpp/h` | Polygon operations |
| `resize_image.cpp/h` | Image resizing with multiple methods |
| `rotate.cpp/h` | Image rotation and scaling |
| `rotsprite.cpp/h` | RotSprite algorithm for pixel art |
| `shift_image.cpp/h` | Pixel shifting with wrap |
| `shrink_bounds.cpp/h` | Bounding box calculation |
