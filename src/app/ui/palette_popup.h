// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/palette_listbox.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class View;
}

namespace app {

  namespace gen {
    class PalettePopup;
  }

  class PalettePopup : public ui::PopupWindow {
  public:
    PalettePopup();

    void showPopup(const gfx::Rect& bounds);

  protected:
    void onPalChange(doc::Palette* palette);
    void onLoadPal();
    void onOpenFolder();

  private:
    gen::PalettePopup* m_popup;
    PaletteListBox m_paletteListBox;
  };

} // namespace app
