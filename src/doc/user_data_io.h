// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <iosfwd>

namespace doc {

  class UserData;

  void write_user_data(std::ostream& os, const UserData& userData);
  UserData read_user_data(std::istream& is);

} // namespace doc
