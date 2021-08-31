// Aseprite    | Copyright (C) 2001-2013  David Capello
// LibreSprite | Copyright (C)      2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_EVENT_H_INCLUDED
#define UI_EVENT_H_INCLUDED
#pragma once

namespace ui {

  class Widget;

  // Base class for every kind of event.
  class Event
  {
  public:
    // Creates a new event specifying that it was generated from the
    // source widget.
    Event(Widget* source);
    virtual ~Event();

    // Returns the widget which generated the event.
    Widget* getSource();

  private:
    // The widget which generates the event.
    Widget* m_source;
  };

} // namespace ui

#endif
