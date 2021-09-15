// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/event.h"
#include <iosfwd>

namespace ui {

  class Widget;

  class SaveLayoutEvent : public Event {
  public:
    SaveLayoutEvent(Widget* source, std::ostream& stream)
      : Event(source)
      , m_stream(stream) {
    }

    std::ostream& stream() { return m_stream; }

  private:
    std::ostream& m_stream;
  };

} // namespace ui
