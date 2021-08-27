// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "recent_files_menu.h"
#include "app.h"
#include "app_menus.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "ui/app_menuitem.h"
#include "base/bind.h"
#include "base/path.h"
#include "recent_files.h"

namespace app {

using namespace ui;

RecentFilesMenu::RecentFilesMenu()
{
  auto& Changed = App::instance()->recentFiles()->Changed;
  m_recentFilesConn = Changed.connect(base::Bind(&RecentFilesMenu::rebuildRecentList, this));
}

void RecentFilesMenu::rebuildRecentList()
{
    auto list_menuitem = app::AppMenus::instance()->getById<MenuItem>("recent_list");
    if (!list_menuitem || list_menuitem->hasSubmenuOpened()) return;
    Command* cmd_open_file = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);

    Menu* submenu = list_menuitem->getSubmenu();
    if (submenu) {
        list_menuitem->setSubmenu(NULL);
        submenu->deferDelete();
    }

    // Build the menu of recent files
    submenu = new Menu();
    list_menuitem->setSubmenu(submenu);

    auto it = App::instance()->recentFiles()->files_begin();
    auto end = App::instance()->recentFiles()->files_end();
    if (it != end) {
        Params params;

        for (; it != end; ++it) {
            const char* filename = it->c_str();
            params.set("filename", filename);

            auto menuitem = new AppMenuItem(
                base::get_file_name(filename).c_str(),
                cmd_open_file,
                params);
            submenu->addChild(menuitem);
        }
    }
    else {
        auto menuitem = new AppMenuItem("Nothing", NULL, Params());
        menuitem->setEnabled(false);
        submenu->addChild(menuitem);
    }
}

}
