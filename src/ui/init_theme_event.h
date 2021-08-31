// Aseprite    | Copyright (C) 2001-2013, 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_INIT_THEME_EVENT_H_INCLUDED
#define UI_INIT_THEME_EVENT_H_INCLUDED
#pragma once

#include "ui/event.h"

namespace ui {

  class Theme;

  class InitThemeEvent : public Event {
  public:
    InitThemeEvent(Widget* source, Theme* theme)
      : Event(source)
      , m_theme(theme){ }

    Theme* theme() const { return m_theme; }

  private:
    Theme* m_theme;
  };

} // namespace ui

#endif  // UI_INIT_THEME_EVENT_H_INCLUDED
