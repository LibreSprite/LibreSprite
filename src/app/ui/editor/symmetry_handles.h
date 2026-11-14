// LibreSprite
// Copyright (C) 2021-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"

#include <algorithm>
#include <vector>

namespace app {

    enum class Axis {
      HORIZONTAL,
      VERTICAL,
      DIAGONAL_45,
      DIAGONAL_135,
      ROTATIONAL_180,
      ROTATIONAL_90
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

    // Clips the infinite line of slope +1 ("\") or -1 ("/") that passes
    // through 'origin' to the boundary of 'rect'. Used to draw/hit-test the
    // diagonal symmetry guide lines, which (unlike the horizontal/vertical
    // ones) aren't axis-aligned so they can't be clipped with a plain
    // rect intersection. Returns false if the line doesn't cross 'rect'.
    inline bool clip_diagonal_symmetry_line(const gfx::Point& origin, int slope,
                                            const gfx::Rect& rect,
                                            gfx::Point& p1, gfx::Point& p2)
    {
      double tMinX = rect.x - origin.x;
      double tMaxX = rect.x + rect.w - origin.x;
      double tMinY, tMaxY;
      if (slope > 0) {
        tMinY = rect.y - origin.y;
        tMaxY = rect.y + rect.h - origin.y;
      }
      else {
        tMinY = origin.y - (rect.y + rect.h);
        tMaxY = origin.y - rect.y;
      }

      double tmin = std::max(tMinX, tMinY);
      double tmax = std::min(tMaxX, tMaxY);
      if (tmin > tmax)
        return false;

      p1 = gfx::Point(origin.x + int(tmin), origin.y + slope*int(tmin));
      p2 = gfx::Point(origin.x + int(tmax), origin.y + slope*int(tmax));
      return true;
    }

} // namespace app
