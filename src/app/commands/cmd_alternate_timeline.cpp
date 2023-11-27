// LibreSprite | Copyright (C)      2023  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "app/ui/main_window.h"
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

class AlternateTimelineCommand : public Command {
public:

  AlternateTimelineCommand();
  Command* clone() const override { return new AlternateTimelineCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
};

AlternateTimelineCommand::AlternateTimelineCommand()
  : Command("AlternateTimeline",
            "Alternate Timeline",
            CmdUIOnlyFlag)
{
}

void AlternateTimelineCommand::onLoadParams(const Params& params){}

bool AlternateTimelineCommand::onEnabled(Context* context){return true;}

void AlternateTimelineCommand::onExecute(Context* context){
    App::instance()->mainWindow()->alternateTimeline();
}

std::string AlternateTimelineCommand::onGetFriendlyName() const {return "Alternate Timeline";}

Command* CommandFactory::createAlternateTimelineCommand()
{
  return new AlternateTimelineCommand;
}

} // namespace app
