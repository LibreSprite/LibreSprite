// SHE library
// Copyright (C) 2012-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace she {

  enum class Capabilities {
    MultipleDisplays = 1,
    CanResizeDisplay = 2,
    DisplayScale = 4,
    GpuAccelerationSwitch = 8,
  };

} // namespace she
