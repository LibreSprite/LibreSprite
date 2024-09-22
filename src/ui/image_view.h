// LibreSprite UI Library
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2024  LibreSprite contributors
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
    ImageView(she::Surface* sur = nullptr, int align = 0, bool disposeSurface = false);
    ~ImageView() {release();}

    she::Surface* getSurface() const {return m_sur;}

    void setSurface(she::Surface* sur, bool disposeSurface) {
      release();
      m_sur = sur;
      m_disposeSurface = disposeSurface;
    }

    void release();

  protected:
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;

  private:
    she::Surface* m_sur{};
    bool m_disposeSurface{};
  };

} // namespace ui
