# Utilities

*Last updated: 2025-12-30*

Helper functions and utilities for common operations.

## Key Utilities

| File | Purpose |
|------|---------|
| `clipboard.cpp/h` | Cross-platform clipboard operations |
| `clipboard_native.cpp/h` | Native OS clipboard integration |
| `autocrop.cpp/h` | Automatically crop transparent borders |
| `expand_cel_canvas.cpp/h` | Expand cel bounds for drawing outside |
| `create_cel_copy.cpp/h` | Deep copy cel with image data |
| `new_image_from_mask.cpp/h` | Create image from selection |
| `range_utils.cpp/h` | Utilities for timeline range selections |
| `freetype_utils.cpp/h` | FreeType font rendering helpers |
| `wrap_value.h` | Value wrapping for tiled mode |

## File Utilities

| File | Purpose |
|------|---------|
| `filetoks.cpp/h` | Filename tokenization |
| `msk_file.cpp/h` | Read/write MSK mask files |
| `pic_file.cpp/h` | Read/write PIC files |

## Clipboard

The clipboard system supports:
- Native OS clipboard (images, text)
- Internal clipboard (preserves layers, frames, palette)
- Copy/paste between documents
