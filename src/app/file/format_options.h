// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace app {

  // Extra options loaded from a file that can be useful to save the
  // file later in the same loaded format (e.g. same color depth, same
  // jpeg quality, etc.).
  class FormatOptions {
  public:
    virtual ~FormatOptions() { }
  };

} // namespace app
