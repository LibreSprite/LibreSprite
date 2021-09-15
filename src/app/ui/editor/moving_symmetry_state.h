// Aseprite    | Copyright (C) 2015-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/pref/preferences.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/symmetry_handles.h"

namespace app {
  class Editor;

  class MovingSymmetryState : public StandbyState {
  public:
    MovingSymmetryState(Editor* editor, ui::MouseMessage* msg,
                        Axis axis,
                        Option<int>& axisPos);
    virtual ~MovingSymmetryState();

    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    virtual bool requireBrushPreview() override { return false; }

  private:
    Axis m_symmetryAxis;
    Option<int>& m_symmetryAxisPos;
    int m_symmetryAxisStart;
    gfx::Point m_mouseStart;
  };

} // namespace app
