// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

#include "home_view.xml.h"

namespace ui {
  class View;
}

namespace app {

  class DataRecoveryView;
  class RecentFilesListBox;
  class RecentFoldersListBox;

  namespace crash {
    class DataRecovery;
  }

  class HomeView : public app::gen::HomeView
                 , public TabView
                 , public WorkspaceView
  {
  public:
    HomeView();
    ~HomeView();

    void showDataRecovery(crash::DataRecovery* dataRecovery);

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onTabPopup(Workspace* workspace) override;
    void onWorkspaceViewSelected() override;

  protected:
    void onResize(ui::ResizeEvent& ev) override;

  private:
    void onNewFile();
    void onOpenFile();
    void onRecoverSprites();

    RecentFilesListBox* m_files;
    RecentFoldersListBox* m_folders;
    crash::DataRecovery* m_dataRecovery;
    DataRecoveryView* m_dataRecoveryView;
  };

} // namespace app
