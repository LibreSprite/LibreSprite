// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <string>
#include <stdexcept>

namespace app {

  class WidgetNotFound : public std::runtime_error {
  public:
    WidgetNotFound(const std::string& widgetId)
      : std::runtime_error("A data file is corrupted.\nPlease reinstall the program\n\n"
                           "Details: Widget not found: " + widgetId) { }
  };

} // namespace app
