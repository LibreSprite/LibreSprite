// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_WIDGET_LOADER_H_INCLUDED
#define APP_WIDGET_LOADER_H_INCLUDED
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
    // Interface used to create customized widgets.
    class IWidgetTypeCreator {
    public:
      virtual ~IWidgetTypeCreator() { }
      virtual void dispose() = 0;
      virtual ui::Widget* createWidgetFromXml(const TiXmlElement* xmlElem) = 0;
    };

    WidgetLoader();
    ~WidgetLoader();

    // Adds a new widget type that can be referenced in the .xml file
    // with an XML element. The "tagName" is the same name as in the
    // .xml should appear as <tagName>...</tagName>
    //
    // The "creator" will not be deleted automatically at the
    // WidgetLoader dtor.
    void addWidgetType(const char* tagName, IWidgetTypeCreator* creator);

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

    typedef std::map<std::string, IWidgetTypeCreator*> TypeCreatorsMap;

    TypeCreatorsMap m_typeCreators;
    ui::TooltipManager* m_tooltipManager;
  };

} // namespace app

#endif
