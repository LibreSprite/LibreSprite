// LibreSprite | Copyright (C)      2023  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

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
#include "app/ui/main_window.h"

namespace app {

  class ToggleTouchbarCommand : public Command {
  public:
    Command* clone() const override { return new ToggleTouchbarCommand(*this); }

    ToggleTouchbarCommand()
      : Command{"ToggleTouchbar", "Toggle Touchbar", CmdUIOnlyFlag}
      {}

  protected:

    void onLoadParams(const Params& params) override {}

    bool onEnabled(Context* context) override {
      return true;
    }

    bool onChecked(Context* context) override {
        return Preferences::instance().touchBar.visible();
    }

    void onExecute(Context* context) override{
      App::instance()->mainWindow()->toggleTouchbar();
    }
  };

  Command* CommandFactory::createToggleTouchbarCommand()
  {
    return new ToggleTouchbarCommand;
  }

} // namespace app
