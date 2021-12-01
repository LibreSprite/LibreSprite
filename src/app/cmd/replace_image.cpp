// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/replace_image.h"

#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/sprite.h"
#include "doc/subobjects_io.h"

namespace app {
namespace cmd {

using namespace doc;

ReplaceImage::ReplaceImage(Sprite* sprite, const std::shared_ptr<Image>& oldImage, const std::shared_ptr<Image>& newImage)
  : WithSprite(sprite)
  , m_oldImageId(oldImage->id())
  , m_newImageId(newImage->id())
  , m_newImage(newImage)
{
}

void ReplaceImage::onExecute()
{
  // Save old image in m_copy. We cannot keep an ImageRef to this
  // image, because there are other undo branches that could try to
  // modify/re-add this same image ID
  auto oldImage = sprite()->getImageRef(m_oldImageId);
  ASSERT(oldImage);
  m_copy.reset(Image::createCopy(oldImage.get()));

  replaceImage(m_oldImageId, m_newImage);
  m_newImage.reset();
}

void ReplaceImage::onUndo()
{
  auto newImage = sprite()->getImageRef(m_newImageId);
  ASSERT(newImage);
  ASSERT(!sprite()->getImageRef(m_oldImageId));
  m_copy->setId(m_oldImageId);

  replaceImage(m_newImageId, m_copy);
  m_copy.reset(Image::createCopy(newImage.get()));
}

void ReplaceImage::onRedo()
{
  auto oldImage = sprite()->getImageRef(m_oldImageId);
  ASSERT(oldImage);
  ASSERT(!sprite()->getImageRef(m_newImageId));
  m_copy->setId(m_newImageId);

  replaceImage(m_oldImageId, m_copy);
  m_copy.reset(Image::createCopy(oldImage.get()));
}

void ReplaceImage::replaceImage(ObjectId oldId, const std::shared_ptr<Image>& newImage)
{
  Sprite* spr = sprite();

  for (Cel* cel : spr->uniqueCels()) {
    if (cel->image()->id() == oldId)
      cel->data()->incrementVersion();
  }

  spr->replaceImage(oldId, newImage);
}

} // namespace cmd
} // namespace app
