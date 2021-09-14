// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>
#include <vector>

namespace base {

  void split_string(const std::string& string,
                    std::vector<std::string>& parts,
                    const std::string& separators);

}
