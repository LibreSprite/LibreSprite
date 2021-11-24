// Aseprite    | Copyright (C) 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/region.h"
#include "ui/events/event.h"

namespace ui {

  class ScrollRegionEvent : public Event {
  public:
    ScrollRegionEvent(Widget* source, gfx::Region& region)
      : Event(source), m_region(region) {
    }

    gfx::Region& region() { return m_region; }

  private:
    gfx::Region& m_region;
  };

} // namespace ui
