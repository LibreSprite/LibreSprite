// LibreSprite
// Copyright (C) 2023-2026 LibreSprite contributors
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
      // Detach the grid from the dialog so ~Widget (below) doesn't also try
      // to delete it (it's owned by the m_grid shared_ptr). Do NOT call
      // removeAllChildren() here: the grid still owns its child widgets (the
      // script's Entry/IntEntry, etc.) and ~Widget will delete them. Calling
      // removeAllChildren() detaches them (removeChild only unparents + frees
      // from the manager, it does not delete), and nothing else owns those
      // widgets (the WidgetObject wrappers hold non-owning pointers), so they
      // leaked — their ui::Timer members stayed registered and tripped
      // ui::Timer::checkNoTimers() on shutdown.
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
      script::Value event;
      event.push_back(id() + "_resize");
      event.push_back((double)size().w);
      event.push_back((double)size().h);
      app::AppScripting::raiseEvent(m_scriptFileName, event);
    }

  void closeWindow(bool raiseEvent, bool notifyManager){
    if (raiseEvent) {
      script::Value event;
      event.push_back(id() + "_close");
      app::AppScripting::raiseEvent(m_scriptFileName, event);
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
