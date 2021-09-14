// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "doc/algorithm/flip_type.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {
  class Image;
}

namespace app {
namespace cmd {
  using namespace doc;

  class FlipImage : public Cmd
                  , public WithImage {
  public:
    FlipImage(Image* image, const gfx::Rect& bounds,
      doc::algorithm::FlipType flipType);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void swap();

    gfx::Rect m_bounds;
    doc::algorithm::FlipType m_flipType;
  };

} // namespace cmd
} // namespace app
