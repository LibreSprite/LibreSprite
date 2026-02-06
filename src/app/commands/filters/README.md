# Filter Commands

*Last updated: 2025-12-30*

This directory contains the implementation of image filter commands and their associated UI components for LibreSprite.

## Overview

Filter commands allow users to apply various image processing effects (color adjustments, convolutions, noise reduction, etc.) to sprites and layers. The architecture separates filter logic from UI, enabling both interactive previews and batch processing.

## Key Classes

### Filter Management

- **FilterManagerImpl** (`filter_manager_impl.h/.cpp`) - Core filter execution engine that manages the application of filters to images. Handles pixel iteration, masking, progress reporting, and transaction management. Implements both `FilterManager` and `FilterIndexedData` interfaces.

- **FilterWindow** (`filter_window.h/.cpp`) - Base modal dialog for filter configuration. Provides channel selection, tiled mode options, and real-time preview integration.

- **FilterPreview** (`filter_preview.h/.cpp`) - Widget that renders a live preview of filter effects in the current editor.

- **FilterWorker** (`filter_worker.h/.cpp`) - Background worker for applying filters without blocking the UI.

- **FilterTargetButtons** (`filter_target_buttons.h/.cpp`) - UI component for selecting which color channels (R, G, B, A) a filter affects.

### Filter Command Implementations

- **cmd_color_curve.cpp** - Color curve adjustment filter with interactive curve editor
- **cmd_convolution_matrix.cpp** - Convolution matrix filter for effects like blur, sharpen, emboss
- **cmd_despeckle.cpp** - Noise reduction filter using median filtering
- **cmd_invert_color.cpp** - Color inversion filter
- **cmd_replace_color.cpp** - Replace one color with another throughout an image

### Supporting Components

- **ColorCurveEditor** (`color_curve_editor.h/.cpp`) - Interactive curve editor widget for the color curve filter
- **ConvolutionMatrixStock** (`convolution_matrix_stock.h/.cpp`) - Predefined convolution matrices (blur, sharpen, edge detect, etc.)

## Architecture

```
FilterWindow (UI)
    |
    v
FilterManagerImpl (Execution)
    |
    v
Filter (from filters/ library)
    |
    v
Image Data
```

Filters operate on image data through the `FilterManager` interface, which provides pixel-by-pixel access with masking support. The `FilterWindow` provides a consistent UI pattern with preview, channel selection, and OK/Cancel buttons.
