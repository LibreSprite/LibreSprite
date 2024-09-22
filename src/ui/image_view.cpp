// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2024  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/image_view.h"

#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

ImageView::ImageView(she::Surface* sur, int align, bool dispose) : Widget{kImageViewWidget} {
  setAlign(align);
  setSurface(sur, dispose);
}

void ImageView::release() {
  if (m_disposeSurface) {
    delete m_sur;
    m_disposeSurface = false;
    m_sur = nullptr;
  }
}

void ImageView::onSizeHint(SizeHintEvent& ev) {
  if (!m_sur)
    return;
  gfx::Rect box;
  getTextIconInfo(&box, NULL, NULL,
    align(), m_sur->width(), m_sur->height());

  ev.setSizeHint(
    gfx::Size(
      box.w + border().width(),
      box.h + border().height()));
}

void ImageView::onPaint(PaintEvent& ev) {
  if (!m_sur)
    return;
  Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();
  gfx::Rect icon;
  getTextIconInfo(
    nullptr, nullptr, &icon, align(),
    m_sur->width(), m_sur->height());

  g->fillRect(bgColor(), bounds);
  g->drawRgbaSurface(m_sur, icon.x, icon.y);
}

} // namespace ui
