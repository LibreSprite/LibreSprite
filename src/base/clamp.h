// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace base {

  template<typename T>
  T clamp(const T& value, const T& low, const T& high) {
    return (value > high ? high:
            (value < low ? low:
                           value));
  }

} // namespace base
