// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "doc/blend_mode.h"
#include "doc/cel.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/image.h"
#include "gfx/rect.h"
#include "render/extra_type.h"

namespace doc {
  class Sprite;
}

namespace app {

  class ExtraCel {
  public:
    ExtraCel();

    void create(doc::Sprite* sprite, const gfx::Rect& bounds, doc::frame_t frame, int opacity);
    void destroy();

    render::ExtraType type() const { return m_type; }
    void setType(render::ExtraType type) { m_type = type; }

    doc::Cel* cel() const { return m_cel.get(); }
    doc::Image* image() const { return m_image.get(); }

    doc::BlendMode blendMode() const { return m_blendMode; }
    void setBlendMode(doc::BlendMode mode) { m_blendMode = mode; }

  private:
    render::ExtraType m_type;
    base::UniquePtr<doc::Cel> m_cel;
    std::shared_ptr<doc::Image> m_image;
    doc::ImageBufferPtr m_imageBuffer;
    doc::BlendMode m_blendMode;

    DISABLE_COPYING(ExtraCel);
  };

typedef std::shared_ptr<ExtraCel> ExtraCelRef;

} // namespace app
