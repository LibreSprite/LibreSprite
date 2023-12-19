// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/script/app_scripting.h"

#include "script/script_object.h"
#include "ui/button.h"
#include "app/script/api/widget_script.h"

class ButtonWidgetScriptObject : public WidgetScriptObject {
    std::string m_text;

public:
    ButtonWidgetScriptObject() {
        addProperty("text",
                    [this]{return m_text;},
                    [this](const std::string& text){
                        if (auto button = getWidget<ui::Button>())
                            button->setText(text);
                        m_text = text;
                        return text;
                    });
    }

    DisplayType getDisplayType() override {return DisplayType::Inline;}

    Handle build() override {
        auto scriptFileName = app::AppScripting::getFileName();
        auto button = new ui::Button(m_text);
        auto handle = button->handle();
        button->Click.connect([=](ui::Event&){
          if (handle) {
            app::AppScripting::raiseEvent(scriptFileName, button->id() + "_click");
          }
        });
        return handle;
    }
};

static script::ScriptObject::Regular<ButtonWidgetScriptObject> _SO("ButtonWidgetScriptObject", {
        "widget" + std::to_string(ui::kButtonWidget)
    });
