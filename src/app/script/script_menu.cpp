// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script_menu.h"
#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/file_system.h"
#include "app/ui/app_menuitem.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/log.h"
#include "base/path.h"
#include "base/fs.h"
#include "base/string.h"
#include "script/script_object.h"
#include "script/engine.h"

namespace app {
using namespace ui;

void scanFolder(const std::string& scriptsDir, Command* cmd_run_script, Menu* parent) {
  auto fs = FileSystemModule::instance();
  auto item = fs->getFileItemFromPath(scriptsDir);
  if (!item)
    return;
  Params params;
  FileItemList list = item->children();
  for (auto child : list) {
    bool isFolder = child->isFolder();
    std::string fullPath = base::fix_path_separators(child->fileName());
    if (!isFolder) {
      bool supported = false;
      auto extension = base::string_to_lower(base::get_file_extension(fullPath));
      for (auto& entry : script::Engine::getRegistry()) {
        if (entry.second.hasFlag(extension)) {
          supported = true;
          break;
        }
      }
      if (!supported)
        continue;
    }
    auto cmd = isFolder ? nullptr : cmd_run_script;
    params.set("filename", fullPath.c_str());
    auto menuitem = new AppMenuItem(child->displayName().c_str(), cmd, params);
    parent->addChild(menuitem);
    if (isFolder) {
      auto menu = new Menu();
      scanFolder(fullPath, cmd_run_script, menu);
      menuitem->setSubmenu(menu);
    }
  }
}

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
  FileSystemModule* fs = FileSystemModule::instance();

  ResourceFinder rf;
  rf.includeUserDir("scripts");
  auto scriptsDir = rf.getFirstOrCreateDefault();
  try {
    if (!base::is_directory(scriptsDir))
      base::make_directory(scriptsDir);
  } catch(...){
    LOG("Could not create scripts directory: %s", scriptsDir.c_str());
  }

  LockFS lock(fs);
  fs->refresh();

  rf.includeDataDir("scripts");
  while (rf.next()) {
    std::string scriptsDir = rf.filename();
    if (!base::is_directory(scriptsDir))
      continue;
    scanFolder(scriptsDir, cmd_run_script, menu);
  }

  return true;
}

}
