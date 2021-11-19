// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "app/cmd/with_image.h"
#include "app/cmd_sequence.h"
#include "doc/image.h"

#include <memory>

namespace app {
namespace cmd {
  using namespace doc;

  class ClearMask : public Cmd
                  , public WithCel {
  public:
    ClearMask(Cel* cel);

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
    std::shared_ptr<Image>  m_copy;
    int m_offsetX, m_offsetY;
    int m_boundsX, m_boundsY;
    color_t m_bgcolor;
  };

} // namespace cmd
} // namespace app
