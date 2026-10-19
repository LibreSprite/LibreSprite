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
#include "app/cmd/set_palette.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "base/log.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include "palette_size.xml.h"

#include <climits>

namespace app {

class PaletteSizeCommand : public Command {
public:
  PaletteSizeCommand();
  Command* clone() const override { return new PaletteSizeCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  void onExecute(Context* context) override;

private:
  int m_size;
};

PaletteSizeCommand::PaletteSizeCommand()
  : Command("PaletteSize",
            "Palette Size",
            CmdRecordableFlag)
{
  m_size = 0;
}

void PaletteSizeCommand::onLoadParams(const Params& params)
{
  m_size = params.get_as<int>("size");
}

std::vector<CommandParamInfo> PaletteSizeCommand::onDescribeParams() const
{
  return {
    {"size", "New palette size in colors. With this, resizes the palette without the dialog.", false, ""},
  };
}

void PaletteSizeCommand::onExecute(Context* context)
{
  // With the size param, resize the palette without the dialog.
  if (m_size > 0) {
    ContextWriter writer(context);
    Sprite* sprite = writer.sprite();
    frame_t frame = writer.frame();
    auto palette = sprite->palette(frame)->clone();

    if (m_size == palette->size())
      return;

    palette->resize(MID(1, m_size, INT_MAX));

    Transaction transaction(context, "Palette Size", ModifyDocument);
    transaction.execute(new cmd::SetPalette(sprite, frame, *palette));
    transaction.commit();

    set_current_palette(palette.get(), false);
    ui::Manager::getDefault()->invalidate();
    return;
  }

  // The dialog would block headless.
  if (!App::instance()->isGui()) {
    LOG("PaletteSize: not available in headless mode\n");
    Console console(context);
    console.printf("PaletteSize: not available in headless mode\n");
    return;
  }

  ContextWriter writer(context);
  Sprite* sprite = writer.sprite();
  frame_t frame = writer.frame();
  auto palette = sprite->palette(frame)->clone();

  app::gen::PaletteSize window;
  window.colors()->setTextf("%d", palette->size());
  window.openWindowInForeground();
  if (window.closer() == window.ok()) {
    int ncolors = window.colors()->textInt();
    if (ncolors == palette->size())
      return;

    palette->resize(MID(1, ncolors, INT_MAX));

    Transaction transaction(context, "Palette Size", ModifyDocument);
    transaction.execute(new cmd::SetPalette(sprite, frame, *palette));
    transaction.commit();

    set_current_palette(palette.get(), false);
    ui::Manager::getDefault()->invalidate();
  }
}

Command* CommandFactory::createPaletteSizeCommand()
{
  return new PaletteSizeCommand;
}

} // namespace app
