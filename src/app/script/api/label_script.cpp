// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "ui/label.h"
#include "app/script/api/widget_script.h"

class LabelWidgetScriptObject : public WidgetScriptObject {
  std::string m_text;

public:
  LabelWidgetScriptObject() {
    addProperty("text",
                [this]{return m_text;},
                [this](const std::string& text){
                  label()->setText(text);
                  m_text = text;
                  return text;
                });
  }

  ui::Label* label() {
    auto label = getWidget<ui::Label>();
    if (!label)
      throw script::ObjectDestroyedException{};
    return label;
  }

  DisplayType getDisplayType() override {return DisplayType::Block;}
  Handle build() override {return new ui::Label(m_text);}
};

static script::ScriptObject::Regular<LabelWidgetScriptObject> _SO("LabelWidgetScriptObject", {
    "widget" + std::to_string(ui::kLabelWidget)
  });
