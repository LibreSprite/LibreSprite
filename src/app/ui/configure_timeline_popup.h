// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/pref/preferences.h"
#include "doc/anidir.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class CheckBox;
  class RadioButton;
  class Slider;
}

namespace app {
  namespace gen {
    class TimelineConf;
  }

  class Document;

  class ConfigureTimelinePopup : public ui::PopupWindow {
  public:
    ConfigureTimelinePopup();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onChangeType();
    void onOpacity();
    void onOpacityStep();
    void onResetOnionskin();
    void onLoopTagChange();
    void onCurrentLayerChange();
    void onPositionChange();

  private:
    void updateWidgetsFromCurrentSettings();
    app::Document* doc();
    DocumentPreferences& docPref();

    app::gen::TimelineConf* m_box;
    bool m_lockUpdates;
  };

} // namespace app
