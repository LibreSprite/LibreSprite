// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "base/path.h"
#include "script/engine_delegate.h"
#include "ui/manager.h"
#include "app/launcher.h"
#include <cstdio>

namespace app {

class OpenScriptsFolder : public Command {
public:
  OpenScriptsFolder();
  Command* clone() const override { return new OpenScriptsFolder(*this); }

protected:
  void onExecute(Context* context) override;
};

OpenScriptsFolder::OpenScriptsFolder()
  : Command("OpenScriptsFolder",
            "Open Scripts Folder",
            CmdRecordableFlag)
{
}

void OpenScriptsFolder::onExecute(Context* context)
{
  ResourceFinder rf;
  rf.includeUserDir(base::join_path("scripts", ".").c_str());
  std::string scriptsDir = rf.getFirstOrCreateDefault();
  launcher::open_folder(scriptsDir);
}

Command* CommandFactory::createOpenScriptsFolderCommand()
{
  return new OpenScriptsFolder;
}

} // namespace app
