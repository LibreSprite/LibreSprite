// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/color.h"
#include "ui/base.h"

namespace she {
  class Font;
}

namespace ui {

  class Graphics;
  class Widget;
  class Window;

  // theme.cpp
  void drawTextBox(Graphics* g, Widget* textbox,
                   int* w, int* h, gfx::Color bg, gfx::Color fg);

} // namespace ui
