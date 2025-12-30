# Editor Components

*Last updated: 2025-12-30*

This directory contains the main canvas editor - the central workspace where users view and edit sprites in LibreSprite.

## Overview

The Editor is the primary interactive surface for sprite editing. It handles rendering, input events, tool interactions, zoom/pan navigation, and coordinates with the rest of the application through a state machine pattern.

## Key Classes

### Core Editor

- **Editor** (`editor.h/.cpp`) - The main editor widget. Handles:
  - Sprite rendering with zoom and scroll
  - Mouse/keyboard/touch input routing
  - Tool activation and drawing operations
  - Onion skinning, grid overlay, and decorators
  - Smooth trackpad panning with LERP interpolation
  - Document observation for auto-refresh

- **EditorView** (`editor_view.h/.cpp`) - Scroll container that wraps the Editor widget

### State Machine

The editor uses a state pattern to handle different interaction modes:

- **EditorState** (`editor_state.h`) - Base class for all editor states
- **EditorStatesHistory** (`editor_states_history.h/.cpp`) - Stack of previous states for back navigation
- **StandbyState** (`standby_state.h/.cpp`) - Default idle state, handles tool selection and basic input
- **DrawingState** (`drawing_state.h/.cpp`) - Active drawing with current tool
- **ScrollingState** (`scrolling_state.h/.cpp`) - Hand tool panning
- **ZoomingState** (`zooming_state.h/.cpp`) - Zoom tool interaction
- **NavigateState** (`navigate_state.h/.cpp`) - Navigation mode
- **PlayState** (`play_state.h/.cpp`) - Animation playback mode
- **SelectBoxState** (`select_box_state.h/.cpp`) - Rectangular selection
- **MovingPixelsState** (`moving_pixels_state.h/.cpp`) - Moving/transforming selected pixels
- **MovingCelState** (`moving_cel_state.h/.cpp`) - Moving cel position
- **MovingSymmetryState** (`moving_symmetry_state.h/.cpp`) - Adjusting symmetry axis
- **StateWithWheelBehavior** (`state_with_wheel_behavior.h/.cpp`) - Base for states with scroll wheel handling

### Pixel Movement & Transformation

- **PixelsMovement** (`pixels_movement.h/.cpp`) - Handles moving, rotating, scaling, and transforming selected pixels
- **TransformHandles** (`transform_handles.h/.cpp`) - UI handles for transformation operations
- **PivotHelpers** (`pivot_helpers.h/.cpp`) - Pivot point calculation utilities

### Visual Elements

- **BrushPreview** (`brush_preview.h/.cpp`) - Renders brush cursor preview
- **EditorDecorator** (`editor_decorator.h`) - Interface for overlay decorations
- **EditorObserver** (`editor_observer.h`) - Observer interface for editor events
- **EditorObservers** (`editor_observers.h/.cpp`) - Observer notification management
- **SymmetryHandles** (`symmetry_handles.h`) - Symmetry axis visual handles
- **Ruler** (`ruler.h`) - Ruler measurement display

### Tool Integration

- **ToolLoopImpl** (`tool_loop_impl.h/.cpp`) - Implements the tool loop interface for drawing operations
- **EditorCustomizationDelegate** (`editor_customization_delegate.h`) - Interface for tool customization

### Utilities

- **HandleType** (`handle_type.h`) - Enum for transformation handle types
- **Glue** (`glue.h`) - Helper functions for coordinate conversion

## Architecture

```
EditorView (scroll container)
    |
    v
Editor (main widget)
    |
    +-- EditorState (current interaction mode)
    |       |
    |       +-- StandbyState, DrawingState, etc.
    |
    +-- BrushPreview (cursor visualization)
    |
    +-- PixelsMovement (selection transforms)
    |
    +-- EditorDecorator (overlays)
```

## Input Flow

1. Input events arrive at `Editor::onProcessMessage()`
2. Events are delegated to the current `EditorState`
3. States handle input and may transition to other states
4. Drawing operations are executed through `ToolLoopImpl`
