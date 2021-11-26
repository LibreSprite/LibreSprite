// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_frame.h"

#include "app/cmd/add_cel.h"
#include "app/document.h"
#include "doc/cel.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

AddFrame::AddFrame(Sprite* sprite, frame_t newFrame)
  : WithSprite(sprite)
  , m_newFrame(newFrame)
  , m_addCel(nullptr)
{
}

void AddFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  app::Document* doc = static_cast<app::Document*>(sprite->document());

  sprite->addFrame(m_newFrame);
  sprite->incrementVersion();

  if (m_addCel) {
    m_addCel->redo();
  }
  else {
    LayerImage* bglayer = sprite->backgroundLayer();
    if (bglayer) {
      std::shared_ptr<Image> bgimage(Image::create(sprite->pixelFormat(), sprite->width(), sprite->height()));
      clear_image(bgimage.get(), doc->bgColor(bglayer));
      Cel* cel = new Cel(m_newFrame, bgimage);
      m_addCel.reset(new cmd::AddCel(bglayer, cel));
      m_addCel->execute(context());
    }
  }

  // Notify observers about the new frame.
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_newFrame);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

void AddFrame::onUndo()
{
  Sprite* sprite = this->sprite();
  app::Document* doc = static_cast<app::Document*>(sprite->document());

  if (m_addCel)
    m_addCel->undo();

  sprite->removeFrame(m_newFrame);
  sprite->incrementVersion();

  // Notify observers about the new frame.
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_newFrame);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveFrame, ev);
}

} // namespace cmd
} // namespace app
