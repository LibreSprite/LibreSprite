// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/rect.h"
#include "gfx/size.h"

namespace ui {

  class ScrollBar;

  void setup_scrollbars(const gfx::Size& scrollableSize,
                        gfx::Rect& viewportArea,
                        Widget& parent,
                        ScrollBar& hbar,
                        ScrollBar& vbar);

} // namespace ui
