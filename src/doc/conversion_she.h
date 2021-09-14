// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace she {
  class Surface;
}

namespace doc {
  class Image;
  class Palette;

  void convert_image_to_surface(const Image* image, const Palette* palette,
    she::Surface* surface,
    int src_x, int src_y, int dst_x, int dst_y, int w, int h);

} // namespace doc
