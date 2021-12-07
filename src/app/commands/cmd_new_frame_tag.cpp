// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_frame_tag.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/transaction.h"
#include "app/ui/frame_tag_window.h"
#include "app/ui/timeline.h"
#include "doc/frame_tag.h"

#include <stdexcept>

namespace app {

using namespace doc;

class NewFrameTagCommand : public Command {
public:
  NewFrameTagCommand();
  Command* clone() const override { return new NewFrameTagCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewFrameTagCommand::NewFrameTagCommand()
  : Command("NewFrameTag",
            "New Frame Tag",
            CmdRecordableFlag)
{
}

bool NewFrameTagCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameTagCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  frame_t from = reader.frame();
  frame_t to = reader.frame();

  auto range = App::instance()->timeline()->range();
  if (range.enabled() &&
      (range.type() == DocumentRange::kFrames ||
       range.type() == DocumentRange::kCels)) {
    from = range.frameBegin();
    to = range.frameEnd();
  }

  std::unique_ptr<FrameTag> frameTag(new FrameTag(from, to));
  FrameTagWindow window(sprite, frameTag.get());
  if (!window.show())
    return;

  window.rangeValue(from, to);
  frameTag->setFrameRange(from, to);
  frameTag->setName(window.nameValue());
  frameTag->setColor(window.colorValue());
  frameTag->setAniDir(window.aniDirValue());

  {
    ContextWriter writer(reader);
    Transaction transaction(writer.context(), "New Frames Tag");
    transaction.execute(new cmd::AddFrameTag(writer.sprite(), frameTag.release()));
    transaction.commit();
  }

  App::instance()->timeline()->invalidate();
}

Command* CommandFactory::createNewFrameTagCommand()
{
  return new NewFrameTagCommand;
}

} // namespace app
