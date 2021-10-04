// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/palette_listbox.h"
#include "app/res/resource.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/palette.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

class PaletteListItem : public ListItem {
public:
  PaletteListItem(std::shared_ptr<doc::Palette> palette, const std::string& name) :
    ListItem(name),
    m_palette(palette) {}

  std::shared_ptr<doc::Palette> palette() {return m_palette;}

  const std::string& name() {return text();}

protected:
  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kMouseLeaveMessage:
      case kMouseEnterMessage:
        invalidate();
        break;
    }
    return ListItem::onProcessMessage(msg);
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    Graphics* g = ev.graphics();
    gfx::Rect bounds = clientBounds();
    gfx::Color bgcolor, fgcolor;
    auto palette = m_palette.get();
    const int leftPadding = 4*guiscale();

    if (isSelected()) {
      bgcolor = theme->colors.listitemSelectedFace();
      fgcolor = theme->colors.listitemSelectedText();
    }
    else {
      bgcolor = theme->colors.listitemNormalFace();
      fgcolor = theme->colors.listitemNormalText();
    }

    g->fillRect(bgcolor, bounds);

    // draw the palette
    gfx::Rect box(
      bounds.x + leftPadding, bounds.y + bounds.h-6*guiscale(),
      4*guiscale(), 4*guiscale());

    int maxColShown = bounds.w / box.w;
    maxColShown = std::min(palette->size(), maxColShown - 5);

    for (int i=0; i<maxColShown; ++i) {
      doc::color_t c = palette->getEntry(i);

      g->fillRect(gfx::rgba(
          doc::rgba_getr(c),
          doc::rgba_getg(c),
          doc::rgba_getb(c)), box);

      box.x += box.w;
    }

    // draw ellipsis if it's too long
    gfx::Color dotsColor = theme->colors.disabled();
    const int ellipsisY = box.y + 2*guiscale();
    if (maxColShown < palette->size()) {
      g->putPixel(dotsColor, box.x + 2*guiscale(), ellipsisY);
      g->putPixel(dotsColor, box.x + 4*guiscale(), ellipsisY);
      g->putPixel(dotsColor, box.x + 6*guiscale(), ellipsisY);
    }

    // and draw the name
    g->drawString(text(), fgcolor, gfx::ColorNone,
      gfx::Point(
        bounds.x + leftPadding,
        bounds.y + bounds.h/2 - g->measureUIString(text()).h/2));
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ev.setSizeHint(gfx::Size(0, (2+16+2)*guiscale()));
  }

private:
  std::shared_ptr<doc::Palette> m_palette;
};

class PaletteListBox::LoadingItem : public ListItem {
public:
  LoadingItem()
    : ListItem("Loading")
    , m_state(0) {
  }

  void makeProgress() {
    std::string text = "Loading ";

    switch ((++m_state) % 4) {
      case 0: text += "/"; break;
      case 1: text += "-"; break;
      case 2: text += "\\"; break;
      case 3: text += "|"; break;
    }

    setText(text);
  }

private:
  int m_state;
};

doc::Palette* PaletteListBox::selectedPalette() {
  if (PaletteListItem* listItem = dynamic_cast<PaletteListItem*>(getSelectedChild()))
    return listItem->palette().get();
  return nullptr;
}

std::string PaletteListBox::selectedPaletteName() {
  if (PaletteListItem* listItem = dynamic_cast<PaletteListItem*>(getSelectedChild()))
    return listItem->name();
  return "";
}

void PaletteListBox::onChange() {
  auto palette = selectedPalette();
  if (palette)
    PalChange(palette);
}

void PaletteListBox::setLoading(bool isLoading) {
  if (isLoading) {
    if (!m_loadingItem) {
      m_loadingItem = new LoadingItem;
      addChild(m_loadingItem);
    }
    m_loadingItem->makeProgress();
  } else {
    if (m_loadingItem) {
      removeChild(m_loadingItem);
      delete m_loadingItem;
      m_loadingItem = nullptr;
      invalidate();
    }
  }
}

void PaletteListBox::addPalette(std::shared_ptr<doc::Palette> palette, const std::string& name) {
  int hasLoading = !!m_loadingItem;
  insertChild(getItemsCount()-hasLoading, new PaletteListItem(palette, name));
  layout();
  View* view = View::getView(this);
  if (view)
    view->updateView();
}

} // namespace app
