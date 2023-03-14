// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "gfx/point.h"
#include "gfx/rect.h"

namespace app {
namespace cmd {

  class CropCel : public Cmd
                , public WithCel {
  public:
    CropCel(std::shared_ptr<doc::Cel> cel, const gfx::Rect& newBounds);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void cropImage(const gfx::Point& origin,
                   const gfx::Rect& bounds);

    gfx::Point m_oldOrigin;
    gfx::Point m_newOrigin;
    gfx::Rect m_oldBounds;
    gfx::Rect m_newBounds;
  };

} // namespace cmd
} // namespace app
