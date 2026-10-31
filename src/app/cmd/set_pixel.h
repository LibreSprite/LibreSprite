// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "doc/color.h"
#include "gfx/point.h"

namespace app {
namespace cmd {
  using namespace doc;

  // Writes one pixel (a raw, format-dependent write: for indexed images
  // the color is a palette index). The old pixel is captured at
  // construction, so onUndo/onRedo just swap the two colors.
  class SetPixel : public Cmd
                 , public WithImage {
  public:
    SetPixel(Image* image, int x, int y, color_t color);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;

  private:
    gfx::Point m_pos;
    color_t m_oldColor;
    color_t m_newColor;
  };

} // namespace cmd
} // namespace app
