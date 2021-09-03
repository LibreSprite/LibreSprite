// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/string.h"
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
  if (base::get_file_path(m_filename).empty()) {
    ResourceFinder rf;
    rf.includeDataDir(base::join_path("scripts", m_filename).c_str());
    if (rf.findFirst())
      m_filename = rf.filename();
  }
}

void RunScriptCommand::onExecute(Context* context)
{
  script::EngineDelegate::setDefault("gui");
  script::Engine::setDefault(base::string_to_lower(base::get_file_extension(m_filename)), false);
  AppScripting engine;
  engine.evalFile(m_filename);

  ui::Manager::getDefault()->invalidate();
}

Command* CommandFactory::createRunScriptCommand()
{
  return new RunScriptCommand;
}

} // namespace app
