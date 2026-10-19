// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "gfx/point.h"

#include <memory>
#include <string>
#include <vector>

namespace app {

  struct ScriptToolLoopConfig;

  namespace tools {
    class Ink;
    class Tool;
  }

  // A synthetic pointer event to replay through the tool pipeline.
  // The point is in sprite space (like tools::Pointer).
  struct ScriptPointerEvent {
    enum Kind { Down, Move, Up };

    gfx::Point point;
    float pressure = 1.0;
    Kind kind = Down;
  };

  // Resolves a tool by its toolbox id (e.g. "pencil").
  // Throws std::runtime_error when the id is unknown.
  tools::Tool* resolve_tool(const std::string& id);

  // Resolves an ink by its toolbox id (e.g. "paint"). An empty id
  // returns nullptr (the tool default ink).
  // Throws std::runtime_error when the id is unknown.
  std::shared_ptr<tools::Ink> resolve_ink(const std::string& id);

  // Runs a whole tool gesture (press/move/release) with synthetic
  // pointer events, like the GUI does with the mouse: it makes the
  // target document active, builds the ScriptToolLoop, replays the
  // events through a ToolLoopManager and disposes the loop.
  //
  // The replay stops when the controller finishes the gesture
  // (releaseButton returns false), as the GUI does.
  //
  // Throws std::runtime_error when the config is invalid (a not-open
  // document, an out-of-range frame, a paint tool without a layer) or
  // when the gesture fails; the gesture is rolled back, so the
  // document is left unchanged.
  void run_tool_events(const ScriptToolLoopConfig& cfg,
                       const std::vector<ScriptPointerEvent>& events);

  // Convenience event sequences for the standard tool gestures.
  // They throw std::runtime_error when there are not enough points.

  // One press, one move per extra point and one release (pencil,
  // eraser, lasso, ...).
  std::vector<ScriptPointerEvent> freehand(const std::vector<gfx::Point>& points);

  // Press at p0, move to p1 and release there (line, rectangle,
  // ellipse, marquees).
  std::vector<ScriptPointerEvent> two_points(const gfx::Point& p0, const gfx::Point& p1);

  // A bezier curve with start p0, control points p1 and p2 and end
  // p3 (curve tool).
  std::vector<ScriptPointerEvent> four_points(const gfx::Point& p0, const gfx::Point& p1,
                                              const gfx::Point& p2, const gfx::Point& p3);

  // One vertex per point; the gesture ends with a click at the last
  // point (polygon tool).
  std::vector<ScriptPointerEvent> point_by_point(const std::vector<gfx::Point>& points);

  // A single press and release (paint bucket, magic wand).
  std::vector<ScriptPointerEvent> one_point(const gfx::Point& p0);

} // namespace app
