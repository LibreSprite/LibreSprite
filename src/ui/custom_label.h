// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/label.h"

namespace ui {

  class CustomLabel : public Label
  {
  public:
    CustomLabel(const std::string& text);

  protected:
    bool onProcessMessage(Message* msg) override;

  };

} // namespace ui
