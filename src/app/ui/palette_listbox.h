// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/res/resources_loader.h"
#include "base/unique_ptr.h"
#include "doc/palette.h"
#include "ui/listbox.h"
#include "ui/timer.h"

namespace app {
  class PaletteResource;

  class PaletteListBox : public ui::ListBox {
  public:
    PaletteListBox() = default;
    PaletteResource* selectedPaletteResource();
    doc::Palette* selectedPalette();
    std::string selectedPaletteName();
    base::Signal1<void, doc::Palette*> PalChange;
    void addPalette(PaletteResource* resource);
    void addPalette(doc::Palette *palette, const std::string& name);

  protected:
    void setLoading(bool isLoading);
    virtual void onChange();

  private:
    class LoadingItem;
    LoadingItem* m_loadingItem = nullptr;
  };

  class PaletteFileListBox : public PaletteListBox {
  public:
    PaletteFileListBox();

  private:
    bool onProcessMessage(ui::Message* msg);
    void onTick();
    void stop();
    std::unique_ptr<ResourcesLoader> m_resourcesLoader;
    ui::Timer m_resourcesTimer;
  };

} // namespace app
