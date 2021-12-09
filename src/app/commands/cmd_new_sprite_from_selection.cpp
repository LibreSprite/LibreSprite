// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/document.h"
#include "app/util/new_image_from_mask.h"
#include "base/path.h"
#include "doc/cel.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/layer.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "doc/document.h"

#include <cstdio>
#include <memory>

namespace app {

using namespace doc;

class NewSpriteFromSelectionCommand : public Command {
public:
  NewSpriteFromSelectionCommand();
  Command* clone() const override { return new NewSpriteFromSelectionCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewSpriteFromSelectionCommand::NewSpriteFromSelectionCommand()
  : Command("NewSpriteFromSelection",
            "New Sprite From Selection",
            CmdUIOnlyFlag)
{
}

bool NewSpriteFromSelectionCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable |
                             ContextFlags::HasVisibleMask);
}

void NewSpriteFromSelectionCommand::onExecute(Context* context)
{
  const Site site = context->activeSite();
  const app::Document* doc = static_cast<const app::Document*>(site.document());
  const Sprite* sprite = site.sprite();
  const Mask* mask = doc->mask();
  std::shared_ptr<Image> image(new_image_from_mask(site, mask));
  if (!image)
    return;

  Palette* palette = sprite->palette(site.frame());

  std::unique_ptr<Sprite> dstSprite(
    Sprite::createBasicSprite(image->pixelFormat(),
                              image->width(),
                              image->height(),
                              palette->size()));

  palette->copyColorsTo(dstSprite->palette(frame_t(0)));

  LayerImage* dstLayer = static_cast<LayerImage*>(dstSprite->folder()->getFirstLayer());
  if (site.layer()->isBackground())
    dstLayer->configureAsBackground(); // Configure layer name as background
  dstLayer->setFlags(site.layer()->flags()); // Copy all flags
  copy_image(dstLayer->cel(frame_t(0))->image(), image.get());

  std::unique_ptr<Document> dstDoc(new Document(dstSprite.release()));
  char buf[1024];
  std::sprintf(buf, "%s-%dx%d-%dx%d",
               base::get_file_title(doc->filename()).c_str(),
               mask->bounds().x, mask->bounds().y,
               mask->bounds().w, mask->bounds().h);
  dstDoc->setFilename(buf);
  dstDoc->setContext(context);
  dstDoc.release();
}

Command* CommandFactory::createNewSpriteFromSelectionCommand()
{
  return new NewSpriteFromSelectionCommand();
}

} // namespace app
