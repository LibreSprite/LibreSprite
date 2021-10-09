// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/button_set.h"
#include "doc/brushes.h"
#include "ui/box.h"
#include "ui/popup_window.h"
#include "ui/tooltips.h"

#include <memory>

namespace app {

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup();

    void setBrush(doc::Brush* brush);
    void regenerate(const gfx::Rect& box);

    void setupTooltips(ui::TooltipManager* tooltipManager) {
      m_tooltipManager = tooltipManager;
    }

    static she::Surface* createSurfaceForBrush(const doc::BrushRef& brush);

  private:
    void onStandardBrush();
    void onBrushChanges();

    ui::TooltipManager* m_tooltipManager;
    ui::VBox m_box;
    ButtonSet m_standardBrushes;
    ButtonSet* m_customBrushes;
  };

} // namespace app
