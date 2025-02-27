// LibreSprite
// Copyright (C) 2025 LibreSprite Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/path.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/document.h"
#include "app/resource_finder.h"
#include "app/ui/main_window.h"
#include "ui/alert.h"
#include "base/path.h"
#include "base/fs.h"

#include <iostream>

namespace app {

class InstallScriptCommand : public Command {
public:
  InstallScriptCommand();
  Command* clone() const override { return new InstallScriptCommand(*this); }

protected:
  bool onEnabled(Context* context) override {
#if defined(__EMSCRIPTEN__)
    return false;
#endif
    return true;
  }

  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_filename;
};

InstallScriptCommand::InstallScriptCommand()
  : Command("InstallScript",
            "InstallScript",
            CmdRecordableFlag)
{
}

void InstallScriptCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
}

void InstallScriptCommand::onExecute(Context* ctx)
{
  std::cout << "Installing " << m_filename << std::endl;

  {
    ResourceFinder rf;
    rf.includeUserDir("scripts");
    auto scriptsDir = rf.getFirstOrCreateDefault();
    try {
      if (!base::is_directory(scriptsDir))
        base::make_directory(scriptsDir);
    } catch(...){
      LOG("Could not create scripts directory: %s", scriptsDir.c_str());
    }
  }

  ResourceFinder rf;
  rf.includeUserDir(base::join_path("scripts", base::get_file_name(m_filename)).c_str());
  auto dest = rf.getFirstOrCreateDefault();

  std::cout << "Copy " << m_filename << " to " << dest << std::endl;
  base::copy_file(m_filename, dest);
  ctx->executeCommand(CommandId::RescanScripts);
}

Command* CommandFactory::createInstallScriptCommand()
{
  return new InstallScriptCommand;
}

} // namespace app
