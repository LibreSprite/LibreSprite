// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace doc {

  class Remap;
  class Palette;

  enum class SortPaletteBy {
    RED,
    GREEN,
    BLUE,
    ALPHA,
    HUE,
    SATURATION,
    VALUE,
    LIGHTNESS,
    LUMA,
  };

  // Creates a Remap to sort the palette. It doesn't apply the remap.
  Remap sort_palette(Palette* palette, SortPaletteBy channel, bool ascending);

} // namespace doc
