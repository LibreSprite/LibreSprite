// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/color.h"
#include "doc/image.h"

#include <vector>

namespace doc {

  class CompressedImage {
  public:
    struct Scanline {
      int x, y, w;
      color_t color;
      Scanline(int y) : x(0), y(y), w(0), color(0) { }
    };

    typedef std::vector<Scanline> Scanlines;
    typedef Scanlines::const_iterator const_iterator;

    // If diffColors is true, it generates one Scanline instance for
    // each different color. If it's false, it generates a scanline
    // for each row of consecutive pixels different than the mask
    // color.
    CompressedImage(const Image* image, bool diffColors);

    const_iterator begin() const { return m_scanlines.begin(); }
    const_iterator end() const { return m_scanlines.end(); }

    PixelFormat pixelFormat() const { return m_image->pixelFormat(); }
    int width() const { return m_image->width(); }
    int height() const { return m_image->height(); }

  private:
    const Image* m_image;
    Scanlines m_scanlines;
  };

} // namespace doc
