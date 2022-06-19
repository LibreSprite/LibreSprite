// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/script/app_scripting.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "script/engine_delegate.h"
#include "ui/box.h"
#include "ui/combobox.h"
#include "ui/label.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {
  class DevConsoleView : public ui::Box
                       , public TabView
                       , public WorkspaceView
                       , public script::EngineDelegate {
  public:
    DevConsoleView();
    ~DevConsoleView();

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    bool canCloneWorkspaceView() override { return true; }
    WorkspaceView* cloneWorkspaceView() override;
    void onWorkspaceViewSelected() override;
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onTabPopup(Workspace* workspace) override;

    // EngineDelegate impl
    virtual void onConsolePrint(const char* text) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onExecuteCommand(const std::string& cmd);
    void onOpenLanguageMenu();

  private:
    class CommmandEntry;

    ui::View m_view;
    ui::TextBox m_textBox;
    ui::HBox m_bottomBox;
    ui::ComboBox m_language;
    ui::Label m_label;
    CommmandEntry* m_entry;
    script::EngineDelegate::Provides m_dev{this};
    AppScripting m_engine;
  };

} // namespace app
