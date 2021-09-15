// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/widget.h"

namespace ui {

  class Separator : public Widget
  {
  public:
    Separator(const std::string& text, int align);

  protected:
    void onPaint(PaintEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
  };

} // namespace ui
