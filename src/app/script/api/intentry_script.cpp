// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "ui/int_entry.h"
#include "app/script/api/widget_script.h"
#include "app/script/app_scripting.h"

namespace script {
  void setStorage(const script::Value& value, const std::string& key, const std::string& domain);
}

class CustomIntEntry : public ui::IntEntry {
    std::string m_fileName;
public:

    CustomIntEntry(const std::string& fileName) : ui::IntEntry(0, 100), m_fileName(fileName) {}

    bool canRaiseEvent = true;

    void setValueSilent(int value) {
        canRaiseEvent = false;
        setValue(value);
        canRaiseEvent = true;
    }

    void onValueChange() override {
        script::setStorage(getValue(), id(), m_fileName);
        if (canRaiseEvent)
            app::AppScripting::raiseEvent(m_fileName, id() + "_change");
    }
};

class IntEntryWidgetScriptObject : public WidgetScriptObject {
public:
    IntEntryWidgetScriptObject() {
        addProperty("min",
                    [this]{return entry()->min();},
                    [this](int min){entry()->setMin(min); return min;});

        addProperty("max",
                    [this]{return entry()->max();},
                    [this](int max){entry()->setMax(max); return max;});

        addProperty("value",
                    [this]{return entry()->getValue();},
                    [this](int value){entry()->setValueSilent(value); return value;});
    }

    CustomIntEntry* entry() {
        return getWidget<CustomIntEntry>();
    }

    DisplayType getDisplayType() override {return DisplayType::Block;}

    Handle build() override {
        return new CustomIntEntry(app::AppScripting::getFileName());
    }
};

static script::ScriptObject::Regular<IntEntryWidgetScriptObject> _SO("IntentryWidgetScriptObject", {
        "widget" + std::to_string(ui::kEntryWidget)
    });
