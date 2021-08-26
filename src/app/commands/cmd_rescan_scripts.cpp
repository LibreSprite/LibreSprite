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

#include <cstdio>

namespace app {

class RescanScriptsCommand : public Command {
public:
  RescanScriptsCommand();
  Command* clone() const override { return new RescanScriptsCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

RescanScriptsCommand::RescanScriptsCommand()
  : Command("RescanScripts",
            "Rescan Scripts",
            CmdRecordableFlag)
{
}

void RescanScriptsCommand::onExecute(Context* context)
{
    app::AppMenus::instance()->rebuildScriptsList();
}

Command* CommandFactory::createRescanScriptsCommand()
{
  return new RescanScriptsCommand;
}

} // namespace app
