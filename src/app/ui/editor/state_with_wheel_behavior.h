// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/editor/editor_state.h"

namespace render {
  class Zoom;
}

namespace app {

  class StateWithWheelBehavior : public EditorState {
  public:
    virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onTouchMagnify(Editor* editor, ui::TouchMessage* msg) override;
  private:
    void setZoom(Editor* editor, const render::Zoom& zoom, const gfx::Point& mousePos);
  };

} // namespace app
