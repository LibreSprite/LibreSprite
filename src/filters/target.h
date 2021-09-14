// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#define TARGET_RED_CHANNEL              1
#define TARGET_GREEN_CHANNEL            2
#define TARGET_BLUE_CHANNEL             4
#define TARGET_ALPHA_CHANNEL            8
#define TARGET_GRAY_CHANNEL             16
#define TARGET_INDEX_CHANNEL            32
#define TARGET_ALL_FRAMES               64
#define TARGET_ALL_LAYERS               128

#define TARGET_ALL_CHANNELS             \
  (TARGET_RED_CHANNEL           |       \
   TARGET_GREEN_CHANNEL         |       \
   TARGET_BLUE_CHANNEL          |       \
   TARGET_ALPHA_CHANNEL         |       \
   TARGET_GRAY_CHANNEL          )

namespace filters {

  typedef int Target;

} // namespace filters
