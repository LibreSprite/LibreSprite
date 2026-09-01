// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2026       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/alert.h"
#include "ui/widget.h"

namespace app {

class RemoveLayerCommand : public Command {
public:
  RemoveLayerCommand();
  Command* clone() const override { return new RemoveLayerCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

RemoveLayerCommand::RemoveLayerCommand()
  : Command("RemoveLayer",
            "Remove Layer",
            CmdRecordableFlag)
{
}

bool RemoveLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsEditable);
}

void RemoveLayerCommand::onExecute(Context* context)
{
  std::string layer_name;
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  Layer* layer(writer.layer());
  {
    // TODO the range of selected layer should be in doc::Site.
    auto range = App::instance()->timeline()->range();
    if (range.enabled()) {
      if (range.layers() == sprite->countLayers()) {
        ui::Alert::show("Error<<You cannot delete all layers.||&OK");
        return;
      }

      bool anyHidden = false;
      for (LayerIndex layer = range.layerEnd(); layer >= range.layerBegin(); --layer) {
        if (!sprite->indexToLayer(layer)->isVisible()) {
          anyHidden = true;
          break;
        }
      }
      if (anyHidden &&
          ui::Alert::show("Warning"
                           "<<One or more of the selected layers are hidden."
                           "<<Do you really want to delete them?"
                           "||&Yes||&No") != 1)
        return;

      Transaction transaction(writer.context(), "Remove Layer");
      DocumentApi api = document->getApi(transaction);
      for (LayerIndex layer = range.layerEnd(); layer >= range.layerBegin(); --layer) {
        api.removeLayer(sprite->indexToLayer(layer));
      }
      transaction.commit();
    }
    else {
      if (sprite->countLayers() == 1) {
        ui::Alert::show("Error<<You cannot delete the last layer.||&OK");
        return;
      }

      if (!layer->isVisible() &&
          ui::Alert::show("Warning"
                           "<<The layer \"%s\" is hidden."
                           "<<Do you really want to delete it?"
                           "||&Yes||&No", layer->name().c_str()) != 1)
        return;

      layer_name = layer->name();

      Transaction transaction(writer.context(), "Remove Layer");
      DocumentApi api = document->getApi(transaction);
      api.removeLayer(layer);
      transaction.commit();
    }
  }
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  if (!layer_name.empty())
    StatusBar::instance()->showTip(1000, "Layer `%s' removed", layer_name.c_str());
  else
    StatusBar::instance()->showTip(1000, "Layers removed");
}

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}

} // namespace app
