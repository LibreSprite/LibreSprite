// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/add_cel.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <memory>

namespace doc {
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class AddFrame : public Cmd
                 , public WithSprite {
  public:
    AddFrame(Sprite* sprite, frame_t frame);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_addCel ? m_addCel->memSize() : 0);
    }

  private:
    void moveFrames(Layer* layer, frame_t fromThis, frame_t delta);

    frame_t m_newFrame;
    std::unique_ptr<AddCel> m_addCel;
  };

} // namespace cmd
} // namespace app
