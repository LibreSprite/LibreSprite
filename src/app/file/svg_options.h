// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/file/format_options.h"

namespace app {

  // Output scale range; must match the slider in
  // data/widgets/svg_options.xml.
  constexpr int kSvgMinScale = 1;
  constexpr int kSvgMaxScale = 32;

  // Data for SVG files
  class SvgOptions : public FormatOptions {
  public:
    SvgOptions(int scale = 1)
      : m_scale(scale) {
    }

    // Output scale (1-32). viewBox stays in sprite pixels.
    int scale() const { return m_scale; }
    void setScale(int scale) { m_scale = scale; }

  private:
    int m_scale;
  };

} // namespace app
