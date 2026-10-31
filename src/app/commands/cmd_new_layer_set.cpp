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
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class NewLayerSetCommand : public Command {
public:
  NewLayerSetCommand();
  Command* clone() const override { return new NewLayerSetCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_name;
};

NewLayerSetCommand::NewLayerSetCommand()
  : Command("NewLayerSet",
            "New Layer Set",
            CmdRecordableFlag)
{
}

void NewLayerSetCommand::onLoadParams(const Params& params)
{
  m_name = params.get("name");
}

std::vector<CommandParamInfo> NewLayerSetCommand::onDescribeParams() const
{
  return {
    {"name", "Name of the new folder. Empty opens the dialog (headless uses \"New Folder\").", false, ""},
  };
}

bool NewLayerSetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewLayerSetCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  std::string name;

  // With a name param, create the folder without the dialog.
  if (!m_name.empty())
    name = m_name;
  else if (!App::instance()->isGui())
    name = "New Folder";
  else {
    // load the window widget
    std::unique_ptr<Window> window(app::load_widget<Window>("new_layer.xml", "new_layer_set"));

    window->openWindowInForeground();

    if (window->closer() != window->findChild("ok"))
      return;

    name = window->findChild("name")->text();
  }

  Layer* layer;
  {
    Transaction transaction(writer.context(), "New Layer");
    layer = document->getApi(transaction).newLayerFolder(sprite);
    transaction.commit();
  }
  layer->setName(name);

  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());
}

Command* CommandFactory::createNewLayerSetCommand()
{
  return new NewLayerSetCommand;
}

} // namespace app
