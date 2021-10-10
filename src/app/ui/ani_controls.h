// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/button_set.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace app {
  class Editor;

  class AniControls : public ButtonSet {
  public:
    void updateUsingEditor(Editor* editor);

  protected:
    AniControls();
    void onRightClick(std::shared_ptr<Item> item) override;

  private:
    void onClickButton();

    const char* getCommandId(int index) const;
    std::string getTooltipFor(int index) const;
  };

} // namespace app
