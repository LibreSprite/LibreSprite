// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/ui/palette_listbox.h"
#include "app/script/api/widget_script.h"
#include "app/script/app_scripting.h"
#include "doc/palette.h"
#include "script/value.h"
#include "script/engine.h"

namespace script {
  void setStorage(const script::Value& value, const std::string& key, const std::string& domain);
}

class CustomPaletteListBox : public app::PaletteListBox {
  std::string m_fileName;
public:

  CustomPaletteListBox(const std::string& fileName) : m_fileName(fileName) {
  }

  void onChange() override {
    script::setStorage(selectedPaletteName(), id(), m_fileName);
    app::AppScripting::raiseEvent(m_fileName, id() + "_change");
  }
};

class PaletteListBoxWidgetScriptObject : public WidgetScriptObject {
public:
  PaletteListBoxWidgetScriptObject() {
    addProperty("selected", [this]{return handle<CustomPaletteListBox>()->selectedPaletteName();});
    addMethod("addPalette", &PaletteListBoxWidgetScriptObject::addPalette);
  }

  script::ScriptObject* addPalette(const std::string& name) {
    auto pal = doc::Palette::create(1);
    auto palSO = getEngine()->getScriptObject(pal.get());
    palSO->setWrapped(pal->handle(), false);
    handle<CustomPaletteListBox>()->addPalette(pal, name);
    return palSO;
  }

  DisplayType getDisplayType() override {return DisplayType::Block;}

  Handle build() override {
    return new CustomPaletteListBox(app::AppScripting::getFileName());
  }
};

static script::ScriptObject::Regular<PaletteListBoxWidgetScriptObject> _SO("PalettelistboxWidgetScriptObject", {
    "widget" + std::to_string(ui::kListItemWidget)
  });
