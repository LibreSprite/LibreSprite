// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>

namespace doc {

  void write_string(std::ostream& os, const std::string& str);
  std::string read_string(std::istream& is);

} // namespace doc
