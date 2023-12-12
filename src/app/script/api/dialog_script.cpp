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

#include "widget_script.h"
#include "script/engine.h"

#include <memory>
#include <list>

namespace ui {
class Dialog;
}

namespace dialog {
using DialogIndex = std::unordered_map<std::string, ui::Dialog*>;

std::shared_ptr<DialogIndex> getDialogIndex() {
  static std::weak_ptr<DialogIndex> dialogs;
  std::shared_ptr<DialogIndex> ptr;
  if (dialogs.expired() || !(ptr = dialogs.lock())) {
    dialogs = ptr = std::make_shared<DialogIndex>();
  }
  return ptr;
}
}

namespace ui {
class Dialog : public ui::Window {
  std::shared_ptr<dialog::DialogIndex> m_index;
public:
  Dialog() : ui::Window(ui::Window::WithTitleBar, "Script") {}

  ~Dialog() {
    if (m_grid) {
      m_grid->removeAllChildren();
      removeChild(m_grid.get());
    }
    unlist();
  }

  void unlist() {
    if (m_index) {
      auto it = m_index->find(id());
      if (it != m_index->end() && it->second == this)
        m_index->erase(it);
    }
  }

  void add(WidgetScriptObject* child) {
    auto ui = static_cast<ui::Widget*>(child->getWrapped());
    if (!ui)
        return;

    if(m_isInline && !m_children.empty()) m_children.back().push_back(ui);
    else m_children.push_back({ui});

    m_isInline = true;
  }

  void addBreak() {
      m_isInline = false;
  }

  void build(){
    if (m_grid)
      return;

    // LibreSprite has closed the window, remove corresponding ScriptObject (this)
    Close.connect([this](ui::CloseEvent&){closeWindow(true, false);});

    if (!id().empty()) {
      m_index = dialog::getDialogIndex();
      m_index->insert({id(), this});
    }

    if (m_grid) {
      m_grid->removeAllChildren();
      removeChild(m_grid.get());
    }

    std::size_t maxColumns = 1;
    for (auto& row : m_children)
      maxColumns = std::max(row.size(), maxColumns);

    m_grid = std::make_shared<ui::Grid>(maxColumns, false);
    addChild(m_grid.get());

    for (auto& row : m_children) {
      auto size = row.size();
      auto span = 1 + (maxColumns - row.size());
      for (size_t i = 0; i < size; ++i) {
        m_grid->addChildInCell(row[i], span, 1, ui::HORIZONTAL | ui::VERTICAL);
        span = 1;
      }
    }

    setVisible(true);
    centerWindow();
    openWindow();
  }

  void closeWindow(bool raiseEvent, bool notifyManager){
    if (raiseEvent)
      app::AppScripting::raiseEvent(m_scriptFileName, id() + "_close");

    if (notifyManager)
        manager()->_closeWindow(this, true);

    unlist();

    app::TaskManager::instance().delayed([handle = handle()]{
      if (auto self = handle.lock())
        delete *self;
    });
  }

private:
  bool m_isInline = false;
  std::list<std::vector<ui::Widget*>> m_children;
  std::string m_scriptFileName = app::AppScripting::getFileName();
  std::shared_ptr<ui::Grid> m_grid;
  inject<script::Engine> m_engine;
  std::unordered_map<std::string, ui::Widget*> m_namedWidgets;
};
}

class DialogScriptObject : public WidgetScriptObject {
  std::unordered_map<std::string, inject<script::ScriptObject>> m_widgets;

  ui::Widget* build() {
    auto dialog = new ui::Dialog();

    // Scripting engine has finished working, build and show the Window
    inject<script::Engine>{}->afterEval([this](bool success){
      auto widget = getWidget();
      if (!widget)
        return;
      auto dialog = getWrapped<ui::Dialog>();
      if (success)
        dialog->build();
      if (!dialog->isVisible()){
        dialog->closeWindow(false, true);
      }
    });

    return dialog;
  }

public:
  DialogScriptObject() {
    addProperty("title",
                [this]{
                  auto widget = static_cast<ui::Dialog*>(getWidget());
                  return widget ? widget->text() : "";
                },
                [this](const std::string& title){
                  auto widget = static_cast<ui::Dialog*>(getWidget());
                  if (widget)
                    widget->setText(title);
                  return title;
                })
      .doc("read+write. Sets the title of the dialog window.");

    addProperty("canClose",
                []{return true;},
                [this](bool canClose){
                  auto widget = static_cast<ui::Dialog*>(getWidget());
                  if (widget && !canClose) {
                    widget->removeDecorativeWidgets();
                  }
                  return canClose;
                })
      .doc("write only. Determines if the user can close the dialog window.");

    addMethod("add", &DialogScriptObject::add);

    addMethod("get", &DialogScriptObject::get);

    addFunction("close", [this]{
      auto widget = static_cast<ui::Dialog*>(getWidget());
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
      auto widget = static_cast<ui::Dialog*>(getWidget());
      if (widget)
        widget->addBreak();
      return true;
    });
  }

  ~DialogScriptObject() {
    auto dialog = static_cast<ui::Dialog*>(getWidget());
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
    auto dialog = static_cast<ui::Dialog*>(getWidget());
    if (!dialog)
      return nullptr;

    if (type.empty() || get(id))
      return nullptr;

    auto cleanType = base::string_to_lower(type); // "lAbEl" -> "label"
    auto unprefixedType = cleanType;
    cleanType[0] = toupper(cleanType[0]);         // "label" -> "Label"
    cleanType += "WidgetScriptObject";            // "Label" -> "LabelWidgetScriptObject"

    inject<script::ScriptObject> widget{cleanType};
    if (!widget)
        return nullptr;

    auto rawPtr = widget.get<WidgetScriptObject>();
    dialog->add(rawPtr);

    auto cleanId = !id.empty() ? id : unprefixedType + std::to_string(m_nextWidgetId++);
    widget->set("id", cleanId);
    m_widgets.emplace(cleanId, std::move(widget));
    return rawPtr;
  }

  uint32_t m_nextWidgetId = 0;
};

static script::ScriptObject::Regular<DialogScriptObject> dialogSO("DialogScriptObject");

namespace dialog {
ui::Widget* getDialogById(const std::string& id) {
    auto index = getDialogIndex();
    auto it = index->find(id);
    return it == index->end() ? nullptr : it->second;
}
}
