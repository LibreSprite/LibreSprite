// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace doc {
  class Image;
  class Mask;

  namespace algorithm {

    void shift_image_with_mask(Image* image, const Mask* mask, int dx, int dy);

  }
}
