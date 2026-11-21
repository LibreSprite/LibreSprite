// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/color.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace app {

  class FileOp;

  // Rasterized SvgFormat output.
  struct SvgDialectRaster {
    int width = 0;
    int height = 0;
    std::vector<doc::color_t> pixels;   // RGBA, row-major, width*height
    bool hasAlpha = false;

    doc::color_t at(int x, int y) const {
      return pixels[size_t(y) * size_t(width) + size_t(x)];
    }
  };

  // Reader for the rectangles SvgFormat writes ("M x y h dx v dy z").
  // Fallback when the system loader has no SVG decoder.
  class SvgDialect {
  public:
    // Return false to cancel.
    typedef std::function<bool(double)> ProgressFn;

    // Parse and rasterize svg. Returns false with error set on failure.
    static bool parse(std::string_view svg,
                      SvgDialectRaster& raster,
                      std::string& error,
                      const ProgressFn& progress = ProgressFn());

    // FileOp wrapper filling the sequence image.
    static bool load(FileOp* fop);
  };

} // namespace app
