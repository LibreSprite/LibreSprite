// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/point.h"

namespace she { class Surface; }

namespace ui {

  class Cursor {
  public:
    // The surface is disposed in ~Cursor.
    Cursor(she::Surface* surface, const gfx::Point& focus);
    ~Cursor();

    she::Surface* getSurface() { return m_surface; }
    const gfx::Point& getFocus() const { return m_focus; }

  private:
    she::Surface* m_surface;
    gfx::Point m_focus;
  };

} // namespace ui
