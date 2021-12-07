// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/widget_type_mismatch.h"

#include <map>
#include <string>

class TiXmlElement;

namespace ui {
  class Widget;
  class TooltipManager;
}

namespace app {

  class WidgetLoader {
  public:
    // Loads the specified widget from an .xml file.
    ui::Widget* loadWidget(const char* fileName, const char* widgetId, ui::Widget* widget = NULL);

    template<class T>
    T* loadWidgetT(const char* fileName, const char* widgetId, T* widget = NULL) {
      T* specificWidget = dynamic_cast<T*>(loadWidget(fileName, widgetId, widget));
      if (!specificWidget)
        throw WidgetTypeMismatch(widgetId);

      return specificWidget;
    }

  private:
    ui::Widget* loadWidgetFromXmlFile(
      const std::string& xmlFilename,
      const std::string& widgetId,
      ui::Widget* widget);

    ui::Widget* convertXmlElementToWidget(const TiXmlElement* elem, ui::Widget* root, ui::Widget* parent, ui::Widget* widget);
    void fillWidgetWithXmlElementAttributes(const TiXmlElement* elem, ui::Widget* root, ui::Widget* widget);
    void fillWidgetWithXmlElementAttributesWithChildren(const TiXmlElement* elem, ui::Widget* root, ui::Widget* widget);

    ui::TooltipManager* m_tooltipManager = nullptr;
  };

} // namespace app
