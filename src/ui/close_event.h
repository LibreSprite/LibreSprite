// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/event.h"

namespace ui {

  class CloseEvent : public Event
  {
  public:
    CloseEvent(Widget* source)
      : Event(source) { }
  };

} // namespace ui
