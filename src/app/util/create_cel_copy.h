// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/frame.h"

namespace doc {
  class Cel;
  class Sprite;
}

namespace app {

  Cel* create_cel_copy(const Cel* srcCel,
                       const Sprite* dstSprite,
                       const frame_t dstFrame);

} // namespace app
