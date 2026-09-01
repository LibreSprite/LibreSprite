// Aseprite    | Copyright (C) 2015       David Capello
// LibreSprite | Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_symmetry_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "ui/message.h"

namespace app {

using namespace ui;

MovingSymmetryState::MovingSymmetryState(Editor* editor, MouseMessage* msg,
                                         Axis axis,
                                         Option<int>& xAxis,
                                         Option<int>& yAxis)
  : m_symmetryAxis(axis)
  , m_xAxis(xAxis)
  , m_yAxis(yAxis)
  , m_xAxisStart(xAxis())
  , m_yAxisStart(yAxis())
{
  m_mouseStart = editor->screenToEditor(msg->position());
  editor->captureMouse();
}

MovingSymmetryState::~MovingSymmetryState()
{
}

bool MovingSymmetryState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSymmetryState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;

  switch (m_symmetryAxis) {
    case Axis::HORIZONTAL:
      m_xAxis(MID(1, m_xAxisStart + delta.x, editor->sprite()->width()-1));
      break;
    case Axis::VERTICAL:
      m_yAxis(MID(1, m_yAxisStart + delta.y, editor->sprite()->height()-1));
      break;
    default:
      // Diagonal and rotational axes share a single draggable origin point,
      // so both coordinates move freely with the mouse.
      m_xAxis(MID(1, m_xAxisStart + delta.x, editor->sprite()->width()-1));
      m_yAxis(MID(1, m_yAxisStart + delta.y, editor->sprite()->height()-1));
      break;
  }

  // Redraw the editor.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSymmetryState::onUpdateStatusBar(Editor* editor)
{
  switch (m_symmetryAxis) {
    case Axis::HORIZONTAL:
      StatusBar::instance()->setStatusText
        (0, "Left %3d Right %3d", m_xAxis(),
         editor->sprite()->width() - m_xAxis());
      break;
    case Axis::VERTICAL:
      StatusBar::instance()->setStatusText
        (0, "Top %3d Bottom %3d", m_yAxis(),
         editor->sprite()->height() - m_yAxis());
      break;
    default:
      StatusBar::instance()->setStatusText
        (0, "Pos %3d %3d", m_xAxis(), m_yAxis());
      break;
  }

  return true;
}

} // namespace app
