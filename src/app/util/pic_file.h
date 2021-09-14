// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace doc {
  class Image;
  class Palette;
}

namespace app {

  doc::Image* load_pic_file(const char* filename, int *x, int *y, doc::Palette** palette);
  int save_pic_file(const char* filename, int x, int y, const doc::Palette* palette, const doc::Image* image);

} // namespace app
