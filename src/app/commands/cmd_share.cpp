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

#if defined(EMSCRIPTEN)
#include "she/system.h"
#include <emscripten/emscripten.h>
#include "base/file_handle.h"
#include "base/path.h"

static void EmscriptenShareFile(const std::string& path) {
  she::instance()->gfx([=]{
    std::vector<uint8_t> bytes;
    auto handle = base::open_file_with_exception(path, "rb");
    FILE* f = handle.get();
    fseek(f, 0, SEEK_END);
    bytes.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    fread(bytes.data(), bytes.size(), 1, f);

    EM_ASM({
	const array = new Uint8Array($1);
	for (let i = 0; i < $1; ++i)
	  array[i] = getValue($0 + i, 'i8');
	const blob = new Blob([array], {type:'application/octet-stream'});
	const url = URL.createObjectURL(blob);
	const a = document.createElement('a');
	a.href = url;
	a.download = UTF8ToString($2);
	a.click();
	URL.revokeObjectURL(url);
      }, bytes.data(), bytes.size(), base::get_file_name(path).c_str());
  });
}
#endif

namespace app {

  class ShareCommand : public Command {
  public:

    ShareCommand() : Command{"Share", "Share", CmdUIOnlyFlag} {}

    Command* clone() const override { return new ShareCommand(*this); }

  protected:
    bool onEnabled(Context* context) override {
#if defined(ANDROID) || defined(__EMSCRIPTEN__)
      return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
#endif
      return false;
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

#if defined(EMSCRIPTEN)
      EmscriptenShareFile(document->filename());
#endif
    }
  };

  Command* CommandFactory::createShareCommand()
  {
    return new ShareCommand;
  }

} // namespace app
