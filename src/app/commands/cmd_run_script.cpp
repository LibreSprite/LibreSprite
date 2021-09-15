// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "base/path.h"
#include "script/engine.h"
#include "script/engine_delegate.h"
#include "ui/manager.h"

#include <cstdio>

namespace app {


class RunScriptCommand : public Command {
public:
  RunScriptCommand();
  Command* clone() const override { return new RunScriptCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_filename;
};

RunScriptCommand::RunScriptCommand()
  : Command("RunScript",
            "Run Script",
            CmdRecordableFlag)
{
}

void RunScriptCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
}

void RunScriptCommand::onExecute(Context* context)
{
  script::EngineDelegate::setDefault("gui");
  AppScripting::evalFile(m_filename);
  ui::Manager::getDefault()->invalidate();
}

Command* CommandFactory::createRunScriptCommand()
{
  return new RunScriptCommand;
}

} // namespace app
