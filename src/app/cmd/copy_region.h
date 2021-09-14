// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "gfx/point.h"
#include "gfx/region.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class CopyRegion : public Cmd
                   , public WithImage {
  public:
    // If alreadyCopied is false, it means that onExecute() will copy
    // pixels from src to dst. If it's true, it means that "onExecute"
    // should do nothing, because modified pixels are alreadt on "dst"
    // (so we use "src" as the original image).
    CopyRegion(Image* dst, const Image* src,
               const gfx::Region& region,
               const gfx::Point& dstPos,
               bool alreadyCopied = false);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_size;
    }

  private:
    void swap();

    size_t m_size;
    bool m_alreadyCopied;
    gfx::Region m_region;
    std::stringstream m_stream;
  };

} // namespace cmd
} // namespace app
