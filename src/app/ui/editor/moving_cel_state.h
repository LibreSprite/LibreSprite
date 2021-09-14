// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/editor/standby_state.h"

#include "app/context_access.h"
#include "doc/cel_list.h"

#include <vector>

namespace doc {
  class Cel;
}

namespace app {
  class Editor;

  class MovingCelState : public StandbyState {
  public:
    MovingCelState(Editor* editor, ui::MouseMessage* msg);
    virtual ~MovingCelState();

    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    virtual bool requireBrushPreview() override { return false; }

  private:
    ContextReader m_reader;
    CelList m_celList;
    std::vector<gfx::Point> m_celStarts;
    gfx::Point m_celOffset;
    gfx::Point m_cursorStart;
    bool m_canceled;
    bool m_maskVisible;
  };

} // namespace app
