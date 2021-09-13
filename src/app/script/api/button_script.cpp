// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/script/app_scripting.h"
#include "base/alive.h"
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
                        if (m_widget)
                            static_cast<ui::Button*>(m_widget)->setText(text);
                        m_text = text;
                        return text;
                    });
    }

    DisplayType getDisplayType() override {return DisplayType::Inline;}
    ui::Widget* build() override {
        auto scriptFileName = app::AppScripting::getFileName();
        auto button = new base::AliveMonitor<ui::Button>(m_text);
        button->Click.connect([=](ui::Event&){
          app::AppScripting::raiseEvent(scriptFileName, button->id() + "_click");
        });
        return button;
    }
};

static script::ScriptObject::Regular<ButtonWidgetScriptObject> _SO("ButtonWidgetScriptObject", {
        "widget" + std::to_string(ui::kButtonWidget)
    });
