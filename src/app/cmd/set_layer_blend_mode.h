// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"
#include "doc/blend_mode.h"

namespace doc {
  class LayerImage;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetLayerBlendMode : public Cmd
                          , public WithLayer {
  public:
    SetLayerBlendMode(LayerImage* layer, BlendMode blendMode);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    BlendMode m_oldBlendMode;
    BlendMode m_newBlendMode;
  };

} // namespace cmd
} // namespace app
