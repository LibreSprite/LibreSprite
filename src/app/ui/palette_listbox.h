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

  class PaletteListBox : public ui::ListBox {
  public:
    PaletteListBox();

    doc::Palette* selectedPalette();
    base::Signal1<void, doc::Palette*> PalChange;

  private:
    bool onProcessMessage(ui::Message* msg);
    void onChange();
    void onTick();
    void stop();

    ResourcesLoader* m_resourcesLoader;
    ui::Timer m_resourcesTimer;

    class LoadingItem;
    LoadingItem* m_loadingItem;
  };

} // namespace app
