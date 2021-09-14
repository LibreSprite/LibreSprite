// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/widget.h"

namespace ui {

  class Box : public Widget
  {
  public:
    Box(int align);

  protected:
    // Events
    void onSizeHint(SizeHintEvent& ev) override;
    void onResize(ResizeEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
  };

  class VBox : public Box
  {
  public:
    VBox() : Box(VERTICAL) { }
  };

  class HBox : public Box
  {
  public:
    HBox() : Box(HORIZONTAL) { }
  };

  class BoxFiller : public Box
  {
  public:
    BoxFiller() : Box(HORIZONTAL) {
      this->setExpansive(true);
    }
  };

} // namespace ui
