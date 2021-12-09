// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "doc/color.h"
#include "doc/image.h"

#include <memory>

namespace app {
namespace cmd {
  using namespace doc;

  class ClearImage : public Cmd
                   , public WithImage {
  public:
    ClearImage(Image* image, color_t color);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + (m_copy ? m_copy->getMemSize() : 0);
    }

  private:
    std::shared_ptr<Image> m_copy;
    color_t m_color;
  };

} // namespace cmd
} // namespace app
