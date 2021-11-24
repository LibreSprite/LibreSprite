// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/events/event.h"

namespace ui {

  class Graphics;
  class Widget;

  class PaintEvent : public Event {
  public:
    PaintEvent(Widget* source, Graphics* graphics);
    virtual ~PaintEvent();

    Graphics* graphics();

    bool isPainted() const;

  private:
    Graphics* m_graphics;
    bool m_painted;
  };

} // namespace ui
