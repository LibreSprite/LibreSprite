// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app {
namespace cmd {
  using namespace doc;

  class ShiftMaskedCel : public Cmd
                       , public WithCel {
  public:
    ShiftMaskedCel(Cel* cel, int dx, int dy);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void shift(int dx, int dy);

    int m_dx, m_dy;
  };

} // namespace cmd
} // namespace app
