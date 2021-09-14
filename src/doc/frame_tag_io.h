// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <iosfwd>

namespace doc {

  class FrameTag;

  void write_frame_tag(std::ostream& os, const FrameTag* tag);
  FrameTag* read_frame_tag(std::istream& is, bool setId = true);

} // namespace doc
