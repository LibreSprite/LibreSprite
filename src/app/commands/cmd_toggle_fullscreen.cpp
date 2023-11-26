// LibreSprite | Copyright (C)      2023  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "she/system.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "she/display.h"

namespace app {

class ToggleFullscreenCommand : public Command {
public:

  ToggleFullscreenCommand();
  Command* clone() const override { return new ToggleFullscreenCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
};

ToggleFullscreenCommand::ToggleFullscreenCommand()
  : Command("ToggleFullscreen",
            "ToggleFullscreen",
            CmdUIOnlyFlag)
{
}

void ToggleFullscreenCommand::onLoadParams(const Params& params){}

bool ToggleFullscreenCommand::onEnabled(Context* context){return true;}

void ToggleFullscreenCommand::onExecute(Context* context){
  she::instance()->defaultDisplay()->toggleFullscreen();
}

std::string ToggleFullscreenCommand::onGetFriendlyName() const {return "Toggle Fullscreen";}

Command* CommandFactory::createToggleFullscreenCommand()
{
  return new ToggleFullscreenCommand;
}

} // namespace app
