// LibreSprite
// Copyright (C) 2021 LibreSprite
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_HANDLE_H_INCLUDED
#define APP_UI_EDITOR_HANDLE_H_INCLUDED
#pragma once

#include "gfx/rect.h"

#include <vector>

namespace app {

    enum Axis {
      HORIZONTAL,
      VERTICAL
    };

    class Handle {
    public:
      Handle(const gfx::Rect& rect, const Axis axis) : m_rect(rect), m_axis(axis) {
      }

      gfx::Rect getRect() { return m_rect; }
      Axis getAxis() { return m_axis; }

    private:
      gfx::Rect m_rect;
      Axis m_axis;
    };

    typedef std::vector<Handle> Handles;

} // namespace app

#endif
