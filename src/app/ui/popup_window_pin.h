// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "ui/button.h"
#include "ui/popup_window.h"

namespace app {

  class PopupWindowPin : public ui::PopupWindow {
  public:
    PopupWindowPin(const std::string& text, ClickBehavior clickBehavior);

  protected:
    virtual bool onProcessMessage(ui::Message* msg) override;
    virtual void onHitTest(ui::HitTestEvent& ev) override;

    // The pin. Your derived class must add this pin in some place of
    // the frame as a children, and you must to remove the pin from the
    // parent in your class's dtor.
    ui::CheckBox* getPin() { return &m_pin; }

  private:
    void onPinClick(ui::Event& ev);

    ui::CheckBox m_pin;
  };

} // namespace app
