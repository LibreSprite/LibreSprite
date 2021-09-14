// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd/add_frame_tag.h"

namespace app {
namespace cmd {
  using namespace doc;

  class RemoveFrameTag : public AddFrameTag {
  public:
    RemoveFrameTag(Sprite* sprite, FrameTag* tag);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
  };

} // namespace cmd
} // namespace app
