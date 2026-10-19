// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/util/msk_file.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/path.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/alert.h"

namespace app {

class SaveMaskCommand : public Command {
  std::string m_filename;

public:
  SaveMaskCommand();
  Command* clone() const override { return new SaveMaskCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SaveMaskCommand::SaveMaskCommand()
  : Command("SaveMask",
            "Save Mask",
            CmdUIOnlyFlag)
{
  m_filename = "";
}

void SaveMaskCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
}

std::vector<CommandParamInfo> SaveMaskCommand::onDescribeParams() const
{
  return {
    {"filename", ".msk mask file to save. Empty opens the save dialog.", false, ""},
  };
}

bool SaveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void SaveMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());

  // Interactive only: the file selector would block headless.
  if (App::instance()->isGui()) {
    std::string filename = (m_filename.empty() ? "default.msk" : m_filename);
    filename = app::show_file_selector(
      "Save .msk File", filename, "msk", FileSelectorType::Save);
    if (filename.empty())
      return;

    m_filename = filename;
  }
  else if (m_filename.empty()) {
    LOG("SaveMask: not available in headless mode without a filename\n");
    Console console;
    console.printf("SaveMask: not available in headless mode without a filename\n");
    return;
  }

  if (save_msk_file(document->mask(), m_filename.c_str()) != 0) {
    if (App::instance()->isGui())
      ui::Alert::show("Error<<Error saving .msk file<<%s||&Close", m_filename.c_str());
    else {
      Console console;
      console.printf("Error saving .msk file: %s\n", m_filename.c_str());
    }
  }
}

Command* CommandFactory::createSaveMaskCommand()
{
  return new SaveMaskCommand;
}

} // namespace app
