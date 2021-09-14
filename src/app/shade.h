// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"

#include <vector>

namespace app {

  typedef std::vector<app::Color> Shade;

  Shade shade_from_string(const std::string& str);
  std::string shade_to_string(const Shade& shade);

} // namespace app
