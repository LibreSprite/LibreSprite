// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_pixel.h"

#include "base/debug.h"
#include "doc/image.h"

namespace app {
namespace cmd {

SetPixel::SetPixel(Image* image, int x, int y, color_t color)
  : WithImage(image)
  , m_pos(x, y)
  , m_oldColor(image->getPixel(x, y))
  , m_newColor(color)
{
  ASSERT(m_pos.x >= 0 && m_pos.x < image->width());
  ASSERT(m_pos.y >= 0 && m_pos.y < image->height());
}

void SetPixel::onExecute()
{
  this->image()->putPixel(m_pos.x, m_pos.y, m_newColor);
  this->image()->incrementVersion();
}

void SetPixel::onUndo()
{
  this->image()->putPixel(m_pos.x, m_pos.y, m_oldColor);
  this->image()->incrementVersion();
}

void SetPixel::onRedo()
{
  this->image()->putPixel(m_pos.x, m_pos.y, m_newColor);
  this->image()->incrementVersion();
}

} // namespace cmd
} // namespace app
