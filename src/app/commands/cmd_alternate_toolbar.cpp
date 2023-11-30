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

  class AlternateToolbarCommand : public Command {
  public:
    Command* clone() const override { return new AlternateToolbarCommand(*this); }

    AlternateToolbarCommand()
      : Command{"AlternateToolbar", "Alternate Toolbar", CmdUIOnlyFlag}
      {}

  protected:

    void onLoadParams(const Params& params) override {}

    bool onEnabled(Context* context) override {
      return true;
    }

    bool onChecked(Context* context) override {
      return Preferences::instance().general.leftToolBar();
    }

    void onExecute(Context* context) override{
      App::instance()->mainWindow()->alternateToolbar();
    }
  };

  Command* CommandFactory::createAlternateToolbarCommand()
  {
    return new AlternateToolbarCommand;
  }

} // namespace app
