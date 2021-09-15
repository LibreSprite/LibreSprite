// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/algorithm/hline.h"
#include "gfx/fwd.h"

namespace doc {

  class Image;
  class Mask;

  namespace algorithm {

    void floodfill(const Image* image,
                   const Mask* mask,
                   int x, int y,
                   const gfx::Rect& bounds,
                   int tolerance, bool contiguous,
                   void* data,
                   AlgoHLine proc);

  }
}
