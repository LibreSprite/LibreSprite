// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "ui/button.h"

namespace app {

  namespace skin {
    class Style;
  }

  class StyledButton : public ui::Button {
  public:
    StyledButton(skin::Style* style);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    skin::Style* m_style;
  };

} // namespace app
