# Filters Library

*Last updated: 2025-12-30*

Image processing filters and effects for sprite manipulation.

## Architecture

All filters implement the `Filter` interface which provides methods for processing RGBA, grayscale, and indexed images row by row.

## Filters

| Filter | Purpose |
|--------|---------|
| `ColorCurveFilter` | Adjust color curves for tonal correction |
| `ConvolutionMatrixFilter` | Kernel-based effects (blur, sharpen, edge detect) |
| `InvertColorFilter` | Invert image colors |
| `MedianFilter` | Noise reduction via median sampling |
| `ReplaceColorFilter` | Replace one color with another |

## Key Classes

- `Filter` - Abstract base interface for all filters
- `FilterManager` - Provides source/destination image access during filtering
- `ConvolutionMatrix` - Defines kernel matrices for convolution operations

## Dependencies

Depends on: base, doc, gfx
