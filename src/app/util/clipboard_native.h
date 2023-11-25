// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "gfx/fwd.h"
#include <memory>

namespace doc {
  class Image;
  class Mask;
  class Palette;
}

namespace app {
namespace clipboard {

void register_native_clipboard_formats();
bool has_native_clipboard_bitmap();
bool set_native_clipboard_bitmap(const doc::Image* image,
                                 const doc::Mask* mask,
                                 const doc::Palette* palette);
bool get_native_clipboard_bitmap(doc::Image** image,
                                 doc::Mask** mask,
                                 std::shared_ptr<doc::Palette>& palette);
bool get_native_clipboard_bitmap_size(gfx::Size* size);

} // namespace clipboard
} // namespace app
