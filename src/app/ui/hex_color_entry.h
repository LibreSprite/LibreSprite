// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"
#include "base/signal.h"
#include "ui/box.h"
#include "ui/entry.h"
#include "ui/label.h"

namespace app {

  // Little widget to show a color in hexadecimal format (as HTML).
  class HexColorEntry : public ui::Box {
  public:
    HexColorEntry();

    void setColor(const app::Color& color);

    // Signals
    base::Signal1<void, const app::Color&> ColorChange;

  protected:
    void onEntryChange();

  private:
    ui::Label m_label;
    ui::Entry m_entry;
  };

} // namespace app
