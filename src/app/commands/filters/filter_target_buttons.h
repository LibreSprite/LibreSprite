// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/button_set.h"
#include "app/ui/skin/skin_part.h"
#include "base/signal.h"
#include "filters/target.h"
#include "ui/tooltips.h"

namespace ui {
  class ButtonBase;
}

namespace app {
  using namespace filters;

  class FilterTargetButtons : public ButtonSet {
  public:
    // Creates a new button to handle "targets" to apply some filter in
    // the a sprite.
    FilterTargetButtons(int imgtype, bool withChannels);

    Target getTarget() const { return m_target; }
    void setTarget(Target target);

    base::Signal0<void> TargetChange;

  protected:
    void onItemChange(Item* item) override;
    void onChannelChange(ui::ButtonBase* button);
    void onImagesChange(ui::ButtonBase* button);

  private:
    void selectTargetButton(Item* item, Target specificTarget);
    void updateFromTarget();
    void updateComponentTooltip(Item* item, const char* channelName, int align);
    skin::SkinPartPtr getCelsIcon() const;

    Target m_target;
    Item* m_red;
    Item* m_green;
    Item* m_blue;
    Item* m_alpha;
    Item* m_gray;
    Item* m_index;
    Item* m_cels;
    ui::TooltipManager m_tooltips;
  };

} // namespace app
