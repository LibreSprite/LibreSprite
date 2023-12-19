// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/string.h"
#include "script/script_object.h"
#include "ui/base.h"
#include "ui/close_event.h"
#include "ui/widget.h"
#include "ui/window.h"
#include <iostream>

#include "base/bind.h"
#include "base/memory.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/script/app_scripting.h"
#include "app/task_manager.h"
#include "app/ui/status_bar.h"
#include "app/ui/dialog.h"
#include "widget_script.h"
#include "script/engine.h"

#include <memory>
#include <list>

namespace ui {
class Dialog;
}

class DialogScriptObject : public WidgetScriptObject {
  std::unordered_map<std::string, inject<script::ScriptObject>> m_widgets;

  Handle build() {
    auto dialog = new ui::Dialog();

    // Scripting engine has finished working, build and show the Window
    getEngine()->afterEval([handle = dialog->handle()](bool success){
      if (auto dialog = handle.get<ui::Widget, ui::Dialog>())
        dialog->build();
    });

    return dialog;
  }

public:
  DialogScriptObject() {
    addProperty("title",
                [this]{
                  auto widget = getWidget<ui::Dialog>();
                  return widget ? widget->text() : "";
                },
                [this](const std::string& title){
                  auto widget = getWidget<ui::Dialog>();
                  if (widget)
                    widget->setText(title);
                  return title;
                })
      .doc("read+write. Sets the title of the dialog window.");

    addProperty("canClose",
                []{return true;},
                [this](bool canClose){
                  auto widget = getWidget<ui::Dialog>();
                  if (widget && !canClose) {
                    widget->removeDecorativeWidgets();
                  }
                  return canClose;
                })
      .doc("write only. Determines if the user can close the dialog window.");

    addMethod("add", &DialogScriptObject::add);

    addMethod("get", &DialogScriptObject::get);

    addFunction("close", [this]{
      auto widget = getWidget<ui::Dialog>();
      if (widget)
        widget->closeWindow(false, true);
      return true;
    });

    addFunction("addDropDown", [this](const std::string& id) {
        auto dropdown = add("dropdown", id);
        return dropdown;
    });

    addFunction("addLabel", [this](const std::string& text, const std::string& id) {
        auto label = add("label", id);
        if (label)
            label->set("text", text);
        return label;
    });

    addFunction("addButton", [this](const std::string& text, const std::string& id) {
        auto button = add("button", id);
        if (button)
            button->set("text", text);
        return button;
    });

    addFunction("addPaletteListBox", [this](const std::string& id) {
        return add("palettelistbox", id);
    });

    addFunction("addIntEntry", [this](const std::string& text, const std::string& id, int min, int max) {
        auto label = add("label", id + "-label");
        if (label)
            label->set("text", text);
        auto intentry = add("intentry", id);
        if (intentry) {
            intentry->set("min", min);
            intentry->set("max", max);
        }
        return intentry;
    });

    addFunction("addEntry", [this](const std::string& text, const std::string& id) {
        return add("entry", id);
    });

    addFunction("addBreak", [this]{
      auto widget = getWidget<ui::Dialog>();
      if (widget)
        widget->addBreak();
      return true;
    });
  }

  ~DialogScriptObject() {
    auto dialog = getWidget<ui::Dialog>();
    if (!dialog)
      return;
    if (!dialog->isVisible())
      dialog->closeWindow(false, false);
  }

  ScriptObject* get(const std::string& id) {
    auto it = m_widgets.find(id);
    return it != m_widgets.end() ? it->second.get() : nullptr;
  }

  ScriptObject* add(const std::string& type, const std::string& id) {
    auto dialog = getWidget<ui::Dialog>();
    if (!dialog)
      return nullptr;

    if (type.empty() || get(id))
      return nullptr;

    auto cleanType = base::string_to_lower(type); // "lAbEl" -> "label"
    auto unprefixedType = cleanType;
    cleanType[0] = toupper(cleanType[0]);         // "label" -> "Label"
    cleanType += "WidgetScriptObject";            // "Label" -> "LabelWidgetScriptObject"

    auto sobj = getEngine()->create(cleanType);
    if (!sobj) {
      return nullptr;
    }

    auto widget = sobj->handle<ui::Widget>();
    if (!widget)
      return nullptr;

    dialog->add(widget);

    auto cleanId = !id.empty() ? id : unprefixedType + std::to_string(m_nextWidgetId++);
    sobj->set("id", cleanId);

    return sobj;
  }

  uint32_t m_nextWidgetId = 0;
};

static script::ScriptObject::Regular<DialogScriptObject> dialogSO(typeid(ui::Dialog*).name());
