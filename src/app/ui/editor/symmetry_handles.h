// LibreSprite
// Copyright (C) 2021 LibreSprite
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_SYMMETRY_HANDLES_H_INCLUDED
#define APP_UI_EDITOR_SYMMETRY_HANDLES_H_INCLUDED
#pragma once

#include "gfx/rect.h"

#include <vector>

namespace app {

    enum Axis {
      HORIZONTAL,
      VERTICAL
    };

    class SymmetryHandle {
    public:
      SymmetryHandle(const gfx::Rect& rect, const Axis axis) : m_rect(rect), m_axis(axis) {
      }

      gfx::Rect rect() { return m_rect; }
      Axis axis() { return m_axis; }

    private:
      gfx::Rect m_rect;
      Axis m_axis;
    };

    typedef std::vector<SymmetryHandle> SymmetryHandles;

} // namespace app

#endif
