// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace doc {

  class Palette;

  namespace file {

    Palette* load_pal_file(const char* filename);
    bool save_pal_file(const Palette* pal, const char* filename);

  } // namespace file
} // namespace doc
