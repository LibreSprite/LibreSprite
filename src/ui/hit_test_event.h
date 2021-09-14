// Aseprite    | Copyright (C) 2001-2013, 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/event.h"

namespace ui {

  enum HitTest {
    HitTestNowhere,
    HitTestCaption,
    HitTestClient,
    HitTestBorderNW,
    HitTestBorderN,
    HitTestBorderNE,
    HitTestBorderE,
    HitTestBorderSE,
    HitTestBorderS,
    HitTestBorderSW,
    HitTestBorderW,
  };

  class HitTestEvent : public Event {
  public:
    HitTestEvent(Widget* source, const gfx::Point& point, HitTest hit)
      : Event(source)
      , m_point(point)
      , m_hit(hit) { }

    gfx::Point point() const { return m_point; }

    HitTest hit() const { return m_hit; }
    void setHit(HitTest hit) { m_hit = hit; }

  private:
    gfx::Point m_point;
    HitTest m_hit;
  };

} // namespace ui
