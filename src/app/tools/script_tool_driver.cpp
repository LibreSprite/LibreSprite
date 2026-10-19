// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/script_tool_driver.h"

#include "app/app.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/tools/ink.h"
#include "app/tools/pointer.h"
#include "app/tools/script_tool_loop.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop.h"
#include "app/tools/tool_loop_manager.h"
#include "app/ui_context.h"
#include "doc/sprite.h"

#include <stdexcept>

namespace app {

namespace {
  // Builds the tools::Pointer for an event with the loop button
  // (pressing the other button would cancel the gesture).
  tools::Pointer make_pointer(const ScriptPointerEvent& ev, tools::ToolLoop::Button button)
  {
    return tools::Pointer(ev.point,
                          (button == tools::ToolLoop::Left ?
                           tools::Pointer::Left:
                           tools::Pointer::Right),
                          ev.pressure);
  }

  ScriptPointerEvent make_event(ScriptPointerEvent::Kind kind, const gfx::Point& point)
  {
    ScriptPointerEvent ev;
    ev.kind = kind;
    ev.point = point;
    return ev;
  }
} // anonymous namespace

tools::Tool* resolve_tool(const std::string& id)
{
  tools::Tool* tool = App::instance()->toolBox()->getToolById(id);
  if (!tool)
    throw std::runtime_error("Unknown tool id: " + id);
  return tool;
}

std::shared_ptr<tools::Ink> resolve_ink(const std::string& id)
{
  if (id.empty())
    return nullptr;

  std::shared_ptr<tools::Ink> ink = App::instance()->toolBox()->getInkById(id);
  if (!ink)
    throw std::runtime_error("Unknown ink id: " + id);
  return ink;
}

void run_tool_events(const ScriptToolLoopConfig& cfg, const std::vector<ScriptPointerEvent>& events)
{
  if (events.empty())
    throw std::runtime_error("run_tool_events: at least one pointer event is needed");

  if (events[0].kind != ScriptPointerEvent::Down)
    throw std::runtime_error("run_tool_events: the first event must be a Down (press)");

  if (!cfg.context || !cfg.document || !cfg.tool)
    throw std::runtime_error(
      "run_tool_events: the context, document and tool must be set in the config");

  // The document must be open (the GUI creates the tool loop from an
  // open document)
  bool open = false;
  for (doc::Document* doc : cfg.context->documents())
    if (doc == cfg.document)
      open = true;
  if (!open)
    throw std::runtime_error("run_tool_events: the document is not open");

  // The frame must be in range
  doc::Sprite* sprite = cfg.document->sprite();
  if (!sprite || cfg.frame >= sprite->totalFrames())
    throw std::runtime_error("run_tool_events: the frame is out of range");

  // Non-selection inks need a layer (as in the GUI tool loop). The
  // ScriptToolLoop constructor validates the layer state and the colors.
  tools::Ink* ink = (cfg.inkOverride ? cfg.inkOverride:
                                       cfg.tool->getInk(cfg.button)).get();
  if (!ink->isSelection() && !cfg.layer)
    throw std::runtime_error(
      "run_tool_events: the tool needs a layer (cfg.layer is null)");

  // The Transaction reads the active document, so make the target
  // document active before the loop is constructed.
  UIContext::instance()->setActiveDocument(cfg.document);

  std::unique_ptr<ScriptToolLoop> loop;
  try {
    loop.reset(new ScriptToolLoop(cfg));
  }
  catch (const std::runtime_error&) {
    // The config validation errors are already descriptive
    throw;
  }

  // The document is read/written across the loop steps, so hold the
  // locks across the gesture. The dispose takes its own locks, as the
  // GUI tool loop does, so it runs outside this scope.
  bool gestureFailed = false;
  std::string failureMessage;
  {
    ContextReader reader(cfg.context, 500);
    ContextWriter writer(reader, 500);

    try {
      tools::ToolLoopManager manager(loop.get());
      manager.prepareLoop(make_pointer(events[0], cfg.button));

      bool finished = false;
      for (const auto& ev : events) {
        if (finished)
          break;

        const tools::Pointer pointer = make_pointer(ev, cfg.button);
        switch (ev.kind) {
          case ScriptPointerEvent::Down:
            manager.pressButton(pointer);
            break;
          case ScriptPointerEvent::Move:
            manager.movement(pointer);
            break;
          case ScriptPointerEvent::Up:
            // When the controller finishes the gesture, the remaining
            // events are ignored (as the GUI does).
            finished = !manager.releaseButton(pointer);
            break;
        }
      }
    }
    catch (const std::exception& ex) {
      gestureFailed = true;
      failureMessage = ex.what();
    }
  }

  if (gestureFailed) {
    // Roll back the gesture and rethrow
    loop->cancel();
    loop->dispose();
    throw std::runtime_error(std::string("run_tool_events: ") + failureMessage);
  }

  loop->dispose();
}

std::vector<ScriptPointerEvent> freehand(const std::vector<gfx::Point>& points)
{
  if (points.empty())
    throw std::runtime_error("freehand: at least one point is needed");

  std::vector<ScriptPointerEvent> events;
  for (size_t i = 0; i < points.size(); ++i)
    events.push_back(make_event(
      (i == 0 ? ScriptPointerEvent::Down: ScriptPointerEvent::Move),
      points[i]));
  events.push_back(make_event(ScriptPointerEvent::Up, points.back()));
  return events;
}

std::vector<ScriptPointerEvent> two_points(const gfx::Point& p0, const gfx::Point& p1)
{
  std::vector<ScriptPointerEvent> events;
  events.push_back(make_event(ScriptPointerEvent::Down, p0));
  events.push_back(make_event(ScriptPointerEvent::Move, p1));
  events.push_back(make_event(ScriptPointerEvent::Up, p1));
  return events;
}

std::vector<ScriptPointerEvent> four_points(const gfx::Point& p0, const gfx::Point& p1,
                                            const gfx::Point& p2, const gfx::Point& p3)
{
  // The controller movements set the curve points in this order: the
  // end point, the two control points, the second control point.
  std::vector<ScriptPointerEvent> events;
  events.push_back(make_event(ScriptPointerEvent::Down, p0));
  events.push_back(make_event(ScriptPointerEvent::Move, p3));
  events.push_back(make_event(ScriptPointerEvent::Up, p3));
  events.push_back(make_event(ScriptPointerEvent::Down, p1));
  events.push_back(make_event(ScriptPointerEvent::Move, p1));
  events.push_back(make_event(ScriptPointerEvent::Up, p1));
  events.push_back(make_event(ScriptPointerEvent::Move, p2));
  events.push_back(make_event(ScriptPointerEvent::Down, p2));
  events.push_back(make_event(ScriptPointerEvent::Up, p2));
  return events;
}

std::vector<ScriptPointerEvent> point_by_point(const std::vector<gfx::Point>& points)
{
  if (points.empty())
    throw std::runtime_error("point_by_point: at least one point is needed");

  // Each press/move/release adds two vertices (the press point and
  // the release point); the gesture ends with a click at the last point.
  std::vector<ScriptPointerEvent> events;
  for (size_t i = 0; i + 1 < points.size(); i += 2) {
    events.push_back(make_event(ScriptPointerEvent::Down, points[i]));
    events.push_back(make_event(ScriptPointerEvent::Move, points[i+1]));
    events.push_back(make_event(ScriptPointerEvent::Up, points[i+1]));
  }
  events.push_back(make_event(ScriptPointerEvent::Down, points.back()));
  events.push_back(make_event(ScriptPointerEvent::Up, points.back()));
  return events;
}

std::vector<ScriptPointerEvent> one_point(const gfx::Point& p0)
{
  std::vector<ScriptPointerEvent> events;
  events.push_back(make_event(ScriptPointerEvent::Down, p0));
  events.push_back(make_event(ScriptPointerEvent::Up, p0));
  return events;
}

} // namespace app
