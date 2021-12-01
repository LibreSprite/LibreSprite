// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/util/msk_file.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/alert.h"

#include <memory>

namespace app {

class LoadMaskCommand : public Command {
  std::string m_filename;

public:
  LoadMaskCommand();
  Command* clone() const override { return new LoadMaskCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

LoadMaskCommand::LoadMaskCommand()
  : Command("LoadMask",
            "LoadMask",
            CmdRecordableFlag)
{
  m_filename = "";
}

void LoadMaskCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
}

bool LoadMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void LoadMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);

  std::string filename = m_filename;

  if (context->isUIAvailable()) {
    filename = app::show_file_selector(
      "Load .msk File", filename, "msk",
      FileSelectorType::Open);

    if (filename.empty())
      return;

    m_filename = filename;
  }

  std::unique_ptr<Mask> mask(load_msk_file(m_filename.c_str()));
  if (!mask)
    throw base::Exception("Error loading .msk file: %s",
                          static_cast<const char*>(m_filename.c_str()));

  {
    ContextWriter writer(reader);
    Document* document = writer.document();
    Transaction transaction(writer.context(), "Mask Load", DoesntModifyDocument);
    transaction.execute(new cmd::SetMask(document, mask.get()));
    transaction.commit();

    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createLoadMaskCommand()
{
  return new LoadMaskCommand;
}

} // namespace app
