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

#include "app/res/palettes_loader_delegate.h"
#include "app/res/palette_resource.h"
#include "app/res/resource.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
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
  PaletteListItem(PaletteResource* palResource)
    : ListItem(palResource->name()), m_palResource(palResource) {
  }

  PaletteResource* paletteResource() const {
    return m_palResource;
  }

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
    doc::Palette* palette = m_palResource->palette();

    if (isSelected()) {
      bgcolor = theme->colors.listitemSelectedFace();
      fgcolor = theme->colors.listitemSelectedText();
    }
    else {
      bgcolor = theme->colors.listitemNormalFace();
      fgcolor = theme->colors.listitemNormalText();
    }

    g->fillRect(bgcolor, bounds);

    gfx::Rect box(
      bounds.x, bounds.y + bounds.h-6*guiscale(),
      4*guiscale(), 4*guiscale());

    for (int i=0; i<palette->size(); ++i) {
      doc::color_t c = palette->getEntry(i);

      g->fillRect(gfx::rgba(
          doc::rgba_getr(c),
          doc::rgba_getg(c),
          doc::rgba_getb(c)), box);

      box.x += box.w;
    }

    g->drawString(text(), fgcolor, gfx::ColorNone,
      gfx::Point(
        bounds.x + guiscale()*2,
        bounds.y + bounds.h/2 - g->measureUIString(text()).h/2));
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ev.setSizeHint(gfx::Size(0, (2+16+2)*guiscale()));
  }

private:
  base::UniquePtr<PaletteResource> m_palResource;
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

PaletteListBox::PaletteListBox()
  : m_resourcesLoader(new ResourcesLoader(new PalettesLoaderDelegate))
  , m_resourcesTimer(100)
  , m_loadingItem(NULL)
{
  m_resourcesTimer.Tick.connect(base::Bind<void>(&PaletteListBox::onTick, this));
}

doc::Palette* PaletteListBox::selectedPalette()
{
  if (PaletteListItem* listItem = dynamic_cast<PaletteListItem*>(getSelectedChild()))
    return listItem->paletteResource()->palette();
  else
    return NULL;
}

bool PaletteListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      m_resourcesTimer.start();
      break;
    }

  }
  return ListBox::onProcessMessage(msg);
}

void PaletteListBox::onChange()
{
  doc::Palette* palette = selectedPalette();
  if (palette)
    PalChange(palette);
}

void PaletteListBox::onTick()
{
  if (m_resourcesLoader == NULL) {
    stop();
    return;
  }

  if (!m_loadingItem) {
    m_loadingItem = new LoadingItem;
    addChild(m_loadingItem);
  }
  m_loadingItem->makeProgress();

  base::UniquePtr<Resource> resource;
  std::string name;

  if (!m_resourcesLoader->next(resource)) {
    if (m_resourcesLoader->isDone()) {
      stop();

      LOG("Done\n");
    }
    return;
  }

  PaletteResource* palResource = static_cast<PaletteResource*>(resource.get());
  base::UniquePtr<PaletteListItem> listItem(new PaletteListItem(palResource));
  insertChild(getItemsCount()-1, listItem);
  layout();

  View* view = View::getView(this);
  if (view)
    view->updateView();

  resource.release();
  listItem.release();
}

void PaletteListBox::stop()
{
  if (m_loadingItem) {
    removeChild(m_loadingItem);
    delete m_loadingItem;
    m_loadingItem = NULL;

    invalidate();
  }

  m_resourcesTimer.stop();
}

} // namespace app
