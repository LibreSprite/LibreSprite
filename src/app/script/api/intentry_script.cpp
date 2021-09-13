// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/alive.h"
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
                    [this]{return getWrapped<ui::IntEntry>()->min();},
                    [this](int min){getWrapped<ui::IntEntry>()->setMin(min); return min;});

        addProperty("max",
                    [this]{return getWrapped<ui::IntEntry>()->max();},
                    [this](int max){getWrapped<ui::IntEntry>()->setMax(max); return max;});

        addProperty("value",
                    [this]{return getWrapped<ui::IntEntry>()->getValue();},
                    [this](int value){getWrapped<CustomIntEntry>()->setValueSilent(value); return value;});
    }

    DisplayType getDisplayType() override {return DisplayType::Block;}

    ui::Widget* build() override {
        return new base::AliveMonitor<CustomIntEntry>(app::AppScripting::getFileName());
    }
};

static script::ScriptObject::Regular<IntEntryWidgetScriptObject> _SO("IntentryWidgetScriptObject", {
        "widget" + std::to_string(ui::kEntryWidget)
    });
