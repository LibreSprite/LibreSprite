// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <iosfwd>

namespace doc {

  class Image;

  void write_image(std::ostream& os, const Image* image);
  Image* read_image(std::istream& is, bool setId = true);

} // namespace doc
