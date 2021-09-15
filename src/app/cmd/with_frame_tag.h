// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/object_id.h"

namespace doc {
  class FrameTag;
}

namespace app {
namespace cmd {
  using namespace doc;

  class WithFrameTag {
  public:
    WithFrameTag(FrameTag* tag);
    FrameTag* frameTag();

  private:
    ObjectId m_frameTagId;
  };

} // namespace cmd
} // namespace app
