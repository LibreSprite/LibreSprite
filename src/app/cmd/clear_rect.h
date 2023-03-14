// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "app/cmd_sequence.h"
#include "doc/image.h"
#include "gfx/fwd.h"

#include <memory>

namespace doc {
  class Cel;
}

namespace app {
namespace cmd {
  using namespace doc;

  class ClearRect : public Cmd {
  public:
    ClearRect(std::shared_ptr<Cel> cel, const gfx::Rect& bounds);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_seq.memSize() +
        (m_copy ? m_copy->getMemSize(): 0);
    }

  private:
    void clear();
    void restore();

    CmdSequence m_seq;
    std::unique_ptr<WithImage> m_dstImage;
    std::shared_ptr<Image> m_copy;
    int m_offsetX, m_offsetY;
    color_t m_bgcolor;
  };

} // namespace cmd
} // namespace app
