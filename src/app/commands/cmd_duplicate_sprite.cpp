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
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/ui_context.h"
#include "base/log.h"
#include "base/path.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "duplicate_sprite.xml.h"

#include <cstdio>

namespace app {

using namespace ui;

class DuplicateSpriteCommand : public Command {
  std::string m_filename;
  std::string m_flatten;

public:
  DuplicateSpriteCommand();
  Command* clone() const override { return new DuplicateSpriteCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateSpriteCommand::DuplicateSpriteCommand()
  : Command("DuplicateSprite",
            "Duplicate Sprite",
            CmdUIOnlyFlag)
{
  m_filename = "";
  m_flatten = "";
}

void DuplicateSpriteCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
  m_flatten = params.get("flatten");
}

std::vector<CommandParamInfo> DuplicateSpriteCommand::onDescribeParams() const
{
  return {
    {"filename", "Destination name for the copy. Default: <title> Copy.<ext>.", false, ""},
    {"flatten", "Flatten layers in the copy. \"true\" or \"false\". Default: false.", false, ""},
  };
}

bool DuplicateSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void DuplicateSpriteCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document = reader.document();

  // Params path: skip the window and run the OK path directly.
  if (!m_filename.empty() || !m_flatten.empty()) {
    std::string filename = m_filename;
    if (filename.empty()) {
      std::string fn = document->filename();
      std::string ext = base::get_file_extension(fn);
      filename = base::get_file_title(fn) +
        " Copy" + (!ext.empty() ? "." + ext: "");
    }

    // Make a copy of the document
    Document* docCopy;
    if (m_flatten == "true")
      docCopy = document->duplicate(DuplicateWithFlattenLayers);
    else
      docCopy = document->duplicate(DuplicateExactCopy);

    docCopy->setFilename(filename.c_str());
    docCopy->setContext(context);
    return;
  }

  // Interactive only: the window would block headless.
  if (!App::instance()->isGui()) {
    LOG("DuplicateSprite: not available in headless mode\n");
    Console console;
    console.printf("DuplicateSprite: not available in headless mode\n");
    return;
  }

  // Load the window widget
  app::gen::DuplicateSprite window;
  std::string fn = document->filename();
  std::string ext = base::get_file_extension(fn);
  window.srcName()->setText(base::get_file_name(fn));
  window.dstName()->setText(base::get_file_title(fn) +
    " Copy" + (!ext.empty() ? "." + ext: ""));

  if (get_config_bool("DuplicateSprite", "Flatten", false))
    window.flatten()->setSelected(true);

  // Open the window
  window.openWindowInForeground();

  if (window.closer() == window.ok()) {
    set_config_bool("DuplicateSprite", "Flatten", window.flatten()->isSelected());

    // Make a copy of the document
    Document* docCopy;
    if (window.flatten()->isSelected())
      docCopy = document->duplicate(DuplicateWithFlattenLayers);
    else
      docCopy = document->duplicate(DuplicateExactCopy);

    docCopy->setFilename(window.dstName()->text().c_str());
    docCopy->setContext(context);
  }
}

Command* CommandFactory::createDuplicateSpriteCommand()
{
  return new DuplicateSpriteCommand;
}

} // namespace app
