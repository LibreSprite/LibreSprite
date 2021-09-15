// Aseprite    | Copyright (C) 2001-2013  David Capello
// LibreSprite | Copyright (C)      2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace ui {

  class Widget;

  // Base class for every kind of event.
  class Event
  {
  public:
    // Creates a new event specifying that it was generated from the
    // source widget.
    Event(Widget* source) : m_source(source) {}
    virtual ~Event() {}

    // Returns the widget which generated the event.
    Widget* getSource() { return m_source; }

  private:
    // The widget which generates the event.
    Widget* m_source;
  };

} // namespace ui
