// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/flip_masked_cel.h"

#include "app/cmd/copy_rect.h"
#include "app/document.h"
#include "app/util/autocrop.h"
#include "doc/algorithm/flip_image.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

FlipMaskedCel::FlipMaskedCel(Cel* cel, doc::algorithm::FlipType flipType)
{
  app::Document* doc = static_cast<app::Document*>(cel->document());
  color_t bgcolor = doc->bgColor(cel->layer());
  Image* image = cel->image();
  Mask* mask = doc->mask();
  ASSERT(mask->bitmap());
  if (!mask->bitmap())
    return;

  std::shared_ptr<Image> copy(Image::createCopy(image));
  int x = cel->x();
  int y = cel->y();
  mask->offsetOrigin(-x, -y);
  doc::algorithm::flip_image_with_mask(
    copy.get(), mask, flipType, bgcolor);
  mask->offsetOrigin(x, y);

  int x1, y1, x2, y2;
  if (get_shrink_rect2(&x1, &y1, &x2, &y2, image, copy.get())) {
    add(new cmd::CopyRect(image, copy.get(),
                          gfx::Clip(x1, y1, x1, y1, x2-x1+1, y2-y1+1)));
  }
}

} // namespace cmd
} // namespace app
