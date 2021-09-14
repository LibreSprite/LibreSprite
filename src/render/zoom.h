// Aseprite Render Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/rect.h"

namespace render {

  class Zoom {
  public:
    Zoom(int num, int den);

    double scale() const {
      return static_cast<double>(m_num) / static_cast<double>(m_den);
    }

    // This value isn't used in operator==() or operator!=()
    double internalScale() const {
      return m_internalScale;
    }

    template<typename T>
    T apply(T x) const {
      return x * m_num / m_den;
    }

    template<typename T>
    T remove(T x) const {
      if (x < 0)
        return (x * m_den / m_num) - 1;
      else
        return (x * m_den / m_num);
    }

    gfx::Rect apply(const gfx::Rect& r) const {
      return gfx::Rect(
        apply(r.x), apply(r.y),
        apply(r.x+r.w) - apply(r.x),
        apply(r.y+r.h) - apply(r.y));
    }
    gfx::Rect remove(const gfx::Rect& r) const {
      return gfx::Rect(
        remove(r.x), remove(r.y),
        remove(r.x+r.w) - remove(r.x),
        remove(r.y+r.h) - remove(r.y));
    }

    void in();
    void out();

    // Returns an linear zoom scale. This position can be incremented
    // or decremented to get a new zoom value.
    int linearScale() const;

    bool operator==(const Zoom& other) const {
      return m_num == other.m_num && m_den == other.m_den;
    }

    bool operator!=(const Zoom& other) const {
      return !operator==(other);
    }

    static Zoom fromScale(double scale);
    static Zoom fromLinearScale(int i);
    static int linearValues();

  private:
    static int findClosestLinearScale(double scale);

    int m_num;
    int m_den;

    // Internal scale value used for precise zooming purposes.
    double m_internalScale;
  };

} // namespace render
