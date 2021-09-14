// Aseprite Gfx Library
// Copyright (C) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/rect.h"
#include <iosfwd>

namespace gfx {

  inline std::ostream& operator<<(std::ostream& os, const Rect& rect)
  {
    return os << "("
              << rect.x << ", "
              << rect.y << ", "
              << rect.w << ", "
              << rect.h << ")";
  }

}
