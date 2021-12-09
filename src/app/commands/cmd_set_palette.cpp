// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_set_palette.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/file_selector.h"
#include "app/ini_file.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "doc/palette.h"
#include "ui/alert.h"
#include "ui/manager.h"

namespace app {

using namespace ui;

SetPaletteCommand::SetPaletteCommand()
  : Command("SetPalette",
            "Set Palette",
            CmdRecordableFlag)
  , m_palette(NULL)
{
}

void SetPaletteCommand::onExecute(Context* context)
{
  ASSERT(m_palette);
  if (!m_palette)
    return;

  ContextWriter writer(context);
  if (writer.document()) {
    Transaction transaction(writer.context(), "Set Palette");
    writer.document()->getApi(transaction)
      .setPalette(writer.sprite(), writer.frame(), m_palette);
    transaction.commit();
  }
  set_current_palette(m_palette, false);

  // Redraw the entire screen
  ui::Manager::getDefault()->invalidate();
}

Command* CommandFactory::createSetPaletteCommand()
{
  return new SetPaletteCommand;
}

} // namespace app
