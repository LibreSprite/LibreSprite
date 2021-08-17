// Aseprite    | Copyright (C) 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
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
                                         Option<int>& axisPos)
  : m_symmetryAxis(axis)
  , m_symmetryAxisPos(axisPos)
  , m_symmetryAxisStart(axisPos())
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
  int pos = 0;

  if (m_symmetryAxis == Axis::HORIZONTAL) {
    pos = m_symmetryAxisStart + delta.x;
    pos = MID(1, pos, editor->sprite()->width()-1);
  }
  else {
    pos = m_symmetryAxisStart + delta.y;
    pos = MID(1, pos, editor->sprite()->height()-1);
  }
  m_symmetryAxisPos(pos);

  // Redraw the editor.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSymmetryState::onUpdateStatusBar(Editor* editor)
{
  if (m_symmetryAxis == Axis::HORIZONTAL)
    StatusBar::instance()->setStatusText
      (0, "Left %3d Right %3d", m_symmetryAxisPos(),
       editor->sprite()->width() - m_symmetryAxisPos());
  else
    StatusBar::instance()->setStatusText
      (0, "Top %3d Bottom %3d", m_symmetryAxisPos(),
       editor->sprite()->height() - m_symmetryAxisPos());

  return true;
}

} // namespace app
