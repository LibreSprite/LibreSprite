// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/widget.h"

namespace she {
  class Surface;
}

namespace ui {

  class ImageView : public Widget {
  public:
    ImageView(she::Surface* sur, int align, bool disposeSurface);
    ~ImageView();

  protected:
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;

  private:
    she::Surface* m_sur;
    bool m_disposeSurface;
  };

} // namespace ui
