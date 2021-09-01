// Aseprite UI Library
// Aseprite    | Copyright (C) 2001-2013, 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/event.h"

namespace ui {

Event::Event(Widget* source)
  : m_source(source)
{
}

Event::~Event()
{
}

Widget* Event::getSource()
{
  return m_source;
}

} // namespace ui
