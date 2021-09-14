// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace ui {

  // Widget types.
  enum WidgetType : int {
    // Undefined (or anonymous) widget type.
    kGenericWidget,

    // Known widgets.
    kBoxWidget,
    kButtonWidget,
    kCheckWidget,
    kComboBoxWidget,
    kEntryWidget,
    kGridWidget,
    kImageViewWidget,
    kLabelWidget,
    kListBoxWidget,
    kListItemWidget,
    kManagerWidget,
    kMenuBarWidget,
    kMenuBoxWidget,
    kMenuItemWidget,
    kMenuWidget,
    kPanelWidget,
    kRadioWidget,
    kSeparatorWidget,
    kSliderWidget,
    kSplitterWidget,
    kTextBoxWidget,
    kViewScrollbarWidget,
    kViewViewportWidget,
    kViewWidget,
    kWindowWidget,

    // User widgets.
    kFirstUserWidget,
    kLastUserWidget = 0x7fffffff
  };

} // namespace ui
