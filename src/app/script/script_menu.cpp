// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script_menu.h"
#include <app/app.h>
#include <app/app_menus.h>
#include <app/commands/command.h>
#include <app/commands/commands.h>
#include <app/commands/params.h>
#include <app/file_system.h>
#include <app/ui/app_menuitem.h>
#include <app/resource_finder.h>
#include <base/bind.h>
#include <base/path.h>
#include "base/fs.h"

namespace app {
using namespace ui;

bool ScriptMenu::rebuildScriptsList(Menu* menu)
{
  // Update the recent file list menu item
  if (!menu)
    return false;

  const WidgetsList& children = menu->children();
  while (children.size() && children.back()->type() != kSeparatorWidget) {
    menu->removeChild(children.back());
  }

  Command* cmd_run_script = CommandsModule::instance()->getCommandByName(CommandId::RunScript);
  ResourceFinder rf;
  rf.includeUserDir("scripts");
  std::string scriptsDir = rf.getFirstOrCreateDefault();

  if (!base::is_directory(scriptsDir))
    base::make_directory(scriptsDir);

  FileSystemModule* fs = FileSystemModule::instance();
  LockFS lock(fs);
  fs->refresh();
  IFileItem* item = fs->getFileItemFromPath(scriptsDir);
  if (item) {
    Params params;
    FileItemList list = item->children();
    for (auto child : list) {
      if (child->isFolder()) {
        continue;
      }
      std::string fullPath = base::fix_path_separators(child->fileName());
      params.set("filename", fullPath.c_str());
      auto menuitem = new AppMenuItem(
        child->displayName().c_str(),
        cmd_run_script,
        params);
      menu->addChild(menuitem);
    }
  }
  return true;
}

}
