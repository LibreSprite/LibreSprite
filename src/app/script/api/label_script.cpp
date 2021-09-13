// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/alive.h"
#include "ui/label.h"
#include "app/script/api/widget_script.h"

class LabelWidgetScriptObject : public WidgetScriptObject {
    std::string m_text;

public:
    LabelWidgetScriptObject() {
        addProperty("text",
                    [this]{return m_text;},
                    [this](const std::string& text){
                        if (m_widget)
                            static_cast<ui::Label*>(m_widget)->setText(text);
                        m_text = text;
                        return text;
                    });
    }

    DisplayType getDisplayType() override {return DisplayType::Block;}
    ui::Widget* build() override {return new base::AliveMonitor<ui::Label>(m_text);}
};

static script::ScriptObject::Regular<LabelWidgetScriptObject> _SO("LabelWidgetScriptObject", {
        "widget" + std::to_string(ui::kLabelWidget)
    });
