// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/weak_set.h"
#include "ui/manager.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace ui {
namespace details {

base::weak_set<Widget> widgets;

void initWidgets() {
  widgets.clear();
}

void exitWidgets() {
  widgets.clear();
}

void addWidget(Widget* widget) {
  widgets.insert(widget->shared_from_this());
}

void removeWidget(Widget* widget) {
  widgets.erase(widget);
}

void resetFontAllWidgets() {
  for (auto widget : widgets) {
    widget->resetFont();
  }
}

void reinitThemeForAllWidgets()
{
  // Reinitialize the theme of each widget
  for (auto widget : widgets) {
    widget->setTheme(CurrentTheme::get());
    widget->initTheme();
  }

  // Remap the windows
  for (auto widget : widgets) {
    if (widget->type() == kWindowWidget)
      std::static_pointer_cast<Window>(widget)->remapWindow();
  }

  // Redraw the whole screen
  Manager::getDefault()->invalidate();
}

} // namespace details
} // namespace ui
