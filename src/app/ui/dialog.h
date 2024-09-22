// LibreSprite
// Copyright (C) 2023 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <memory>
#include <list>
#include <vector>

#include "app/script/app_scripting.h"
#include "app/task_manager.h"
#include "script/value.h"
#include "ui/base.h"
#include "ui/grid.h"
#include "ui/ui.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace ui {
class Dialog : public ui::Window {
public:
  Dialog() : ui::Window(ui::Window::WithTitleBar, "Script") {}

  ~Dialog() {
    if (m_grid) {
      m_grid->removeAllChildren();
      // m_grid might have been removed if the dialog was already closed
      if (m_grid->parent())
        removeChild(m_grid.get());
    }
  }

  void add(Widget* ui) {
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

    if (m_grid) {
      m_grid->removeAllChildren();
      removeChild(m_grid.get());
      m_grid.reset();
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

  void onWindowResize() override {
    app::AppScripting::raiseEvent(m_scriptFileName, {id() + "_resize", size().w, size().h});
  }

  void closeWindow(bool raiseEvent, bool notifyManager){
    if (raiseEvent) {
      app::AppScripting::raiseEvent(m_scriptFileName, {id() + "_close"});
    }
    if (notifyManager) {
      manager()->_closeWindow(this, true);
    }
  }

private:
  bool m_isInline = false;
  std::list<std::vector<ui::Widget*>> m_children;
  std::string m_scriptFileName = app::AppScripting::getFileName();
  std::shared_ptr<ui::Grid> m_grid;
  std::unordered_map<std::string, ui::Widget*> m_namedWidgets;
};
}
