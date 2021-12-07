// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/image.h"

#include <memory>
#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class ReplaceImage : public Cmd
                     , public WithSprite {
  public:
    ReplaceImage(Sprite* sprite, const std::shared_ptr<Image>& oldImage, const std::shared_ptr<Image>& newImage);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_copy ? m_copy->getMemSize(): 0);
    }

  private:
    void replaceImage(ObjectId oldId, const std::shared_ptr<Image>& newImage);

    ObjectId m_oldImageId;
    ObjectId m_newImageId;

    // Reference used only to keep the copy of the new image from the
    // ReplaceImage() ctor until the ReplaceImage::onExecute() call.
    // Then the reference is not used anymore.
    std::shared_ptr<Image> m_newImage;
    std::shared_ptr<Image> m_copy;
  };

} // namespace cmd
} // namespace app
