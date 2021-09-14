// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"

namespace app {
namespace cmd {
  using namespace doc;

  class MoveLayer : public Cmd {
  public:
    MoveLayer(Layer* layer, Layer* afterThis);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    WithLayer m_layer;
    WithLayer m_oldAfterThis;
    WithLayer m_newAfterThis;
  };

} // namespace cmd
} // namespace app
