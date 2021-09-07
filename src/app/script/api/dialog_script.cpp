// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "ui/base.h"
#include "ui/label.h"
#include "ui/widget.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/bind.h"
#include "base/memory.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/script/app_scripting.h"
#include "app/ui/status_bar.h"

#include "widget_script.h"
#include "script/engine.h"

#include <memory>
#include <list>

class Dialog;

using DialogSet = std::unordered_set<std::shared_ptr<Dialog>>;

DialogSet& getDialogSet() {
  static DialogSet dialogs;
  return dialogs;
}

class DialogWindow : public ui::Window {
public:
  DialogWindow(const std::string& text) : ui::Window(ui::Window::WithTitleBar, text) {}

  virtual ~DialogWindow() {onShutdown();}

  std::function<void()> onShutdown;
};

class Dialog : public std::enable_shared_from_this<Dialog> {
public:
  virtual ~Dialog() {
    // can't delete window if there is no manager!
    if (m_window && m_window->manager())
        delete m_window;
  }

  void show() {init();}

  void addLabel(const std::string& text, const std::string& id) {
    init();
    auto label = new ui::Label(text);
    if (!id.empty()) m_namedWidgets[id] = label;
    m_children.push_back({label});
    m_isInline = false;
  }

  void addButton(const std::string& text, const std::string& id) {
    init();
    auto button = new ui::Button(text);

    if(m_isInline) m_children.back().push_back(button);
    else m_children.push_back({button});
    m_isInline = true;

    if (!id.empty()) {
      m_namedWidgets[id] = button;
      button->Click.connect(base::Bind<void>(&Dialog::click, this, id));
    }
  }

  std::string title;

private:
  void click(const std::string& str){
    std::cout << "Clicked " << str << std::endl;
    app::AppScripting::raiseEvent(m_scriptFileName, str + "_click");
  }

  void init() {
    if (m_wasInit) return;
    m_wasInit = true;

    m_window = new DialogWindow(title);
    m_window->Close.connect(base::Bind<void>(&Dialog::closeWindow, this));

    std::weak_ptr<Dialog> weak = shared_from_this();

    // LibreSprite/User has closed the window, remove self
    m_window->onShutdown = [weak]{
      if (auto self = weak.lock()) {
        self->m_window = nullptr;
        self->closeWindow();
      }
    };

    // Scripting engine has finished working, build and show the Window
    m_engine->afterEval([weak]{
      if (auto locked = weak.lock())
        locked->build();
    });
  }

  void build(){
    if (m_grid)
      m_window->removeChild(m_grid);

    std::size_t maxColumns = 1;
    for (auto& row : m_children)
      maxColumns = std::max(row.size(), maxColumns);

    m_grid = new ui::Grid(maxColumns, false);
    m_window->addChild(m_grid);

    for (auto& row : m_children) {
      auto size = row.size();
      auto span = 1 + (maxColumns - row.size());
      for (size_t i = 0; i < size; ++i) {
        m_grid->addChildInCell(row[i], span, 1, ui::HORIZONTAL | ui::VERTICAL);
        span = 1;
      }
    }

    m_window->setVisible(true);
    m_window->centerWindow();
    m_window->openWindow();
  }

  void closeWindow(){
    getDialogSet().erase(shared_from_this());
  }

  bool m_isInline = false;
  std::list<std::vector<ui::Widget*>> m_children;
  std::string m_scriptFileName = app::AppScripting::getFileName();
  DialogWindow* m_window = nullptr;
  ui::Grid* m_grid = nullptr;
  bool m_wasInit = false;
  inject<script::Engine> m_engine;
  std::unordered_map<std::string, ui::Widget*> m_namedWidgets;
};

class DialogScriptObject : public WidgetScriptObject {
  std::shared_ptr<Dialog> m_dialog = std::make_shared<Dialog>();

public:
  DialogScriptObject() {
    getDialogSet().emplace(m_dialog);
    setWrapped(m_dialog.get());
    addProperty("title",
                [this]{return m_dialog->title;},
                [this](const std::string& title){
                  m_dialog->title = title;
                  return 0;
                })
      .documentation("Read+Write. Sets the title the dialog will be created with");
    addMethod("show", m_dialog.get(), &Dialog::show);
    addMethod("addLabel", m_dialog.get(), &Dialog::addLabel);
    addMethod("addButton", m_dialog.get(), &Dialog::addButton);
  }
};

static script::ScriptObject::Regular<DialogScriptObject> dialogSO("DialogScriptObject");
