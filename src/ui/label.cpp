// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/font.h"
#include "ui/events/size_hint_event.h"
#include "ui/label.h"
#include "ui/message.h"
#include "ui/theme.h"

namespace ui {

Label::Label(const std::string& text)
  : Widget(kLabelWidget)
{
  setAlign(LEFT | MIDDLE);
  setText(text);
  initTheme();
}

void Label::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size sz(0, 0);

  if (hasText()) {
    // Labels are not UIString
    sz.w = font()->textLength(text().c_str());
    sz.h = textHeight();
  }

  sz.w += border().width();
  sz.h += border().height();

  ev.setSizeHint(sz);
}

void Label::onPaint(PaintEvent& ev)
{
  theme()->paintLabel(ev);
}

} // namespace ui
