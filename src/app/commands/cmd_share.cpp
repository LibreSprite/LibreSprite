// LibreSprite | Copyright (C)      2023  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "ui/alert.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/context.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "she/display.h"

#if defined(ANDROID)
extern void _AndroidShareFile(const std::string& path);
#endif

namespace app {

  class ShareCommand : public Command {
  public:

    ShareCommand() : Command{"Share", "Share", CmdUIOnlyFlag} {}

    Command* clone() const override { return new ShareCommand(*this); }

  protected:
    bool onEnabled(Context* context) override {
#if defined(ANDROID)
      return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
#else
      return false;
#endif
    }

    void onExecute(Context* context) override {
      auto document = context->activeDocument();

      if (!document->isAssociatedToFile() || document->isModified()) {
        ui::Alert::show("Notice<<You must save before you share!||&OK");
        auto saveAsCommand = app::CommandsModule::instance()->getCommandByName(CommandId::SaveFile);
        context->executeCommand(saveAsCommand);
        return;
      }

#if defined(ANDROID)
      _AndroidShareFile(document->filename());
#endif
    }
  };

  Command* CommandFactory::createShareCommand()
  {
    return new ShareCommand;
  }

} // namespace app
