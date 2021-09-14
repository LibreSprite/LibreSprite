// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <sstream>

namespace doc {
  class Palette;
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class AddPalette : public Cmd
                   , public WithSprite {
  public:
    AddPalette(Sprite* sprite, Palette* pal);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_size;
    }

  private:
    size_t m_size;
    std::stringstream m_stream;
    frame_t m_frame;
  };

} // namespace cmd
} // namespace app
