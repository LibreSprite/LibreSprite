// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>

namespace doc {

  enum class AniDir {
    FORWARD = 0,
    REVERSE = 1,
    PING_PONG = 2,
  };

  std::string convert_to_string(AniDir anidir);

} // namespace doc
