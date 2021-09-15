// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "ui/menu.h"

namespace app {

  class MainMenuBar : public ui::MenuBar {
  public:
    MainMenuBar();

    void reload();
  };

} // namespace app
