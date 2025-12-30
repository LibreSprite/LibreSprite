# Tools

*Last updated: 2025-12-30*

Drawing and editing tools (pencil, brush, fill, selection, etc.).

## Architecture

Tools are composed of several components:
- **Controller** - Handles mouse/pen input interpretation
- **Ink** - Defines how pixels are applied (normal, blur, shading)
- **PointShape** - Shape of the tool tip (pixel, brush, spray)
- **Intertwine** - How strokes are connected (freehand, line, rectangle)

## Key Classes

| Class | Purpose |
|-------|---------|
| `ToolBox` | Registry of all available tools |
| `Tool` | Individual tool definition |
| `ToolLoop` | Active drawing session state |
| `ToolLoopManager` | Coordinates tool execution |
| `ActiveToolManager` | Tracks currently selected tool |

## Tool Components

| Component | Purpose |
|-----------|---------|
| `Controller` | Input handling (one-point, two-point, four-point) |
| `Ink` | Pixel application mode (paint, erase, blur, shade) |
| `PointShape` | Tool tip shape (pixel, brush, spray, floodfill) |
| `Intertwine` | Stroke connection (freehand, line, rect, ellipse) |

## Key Files

| File | Purpose |
|------|---------|
| `tool_box.cpp/h` | Tool registry and initialization |
| `tool_loop.h` | Drawing session interface |
| `tool_loop_manager.cpp/h` | Tool execution coordinator |
| `inks.h` | All ink type implementations |
| `controllers.h` | All controller implementations |
| `point_shapes.h` | All point shape implementations |
| `intertwiners.h` | All intertwine implementations |
| `symmetry.cpp/h` | Symmetry mode (mirror, tiled) |
