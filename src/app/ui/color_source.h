// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"
#include "gfx/point.h"

namespace app {

  class IColorSource {
  public:
    virtual ~IColorSource() { }
    virtual app::Color getColorByPosition(const gfx::Point& pos) = 0;
  };

} // namespace app
