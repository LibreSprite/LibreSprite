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
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/transaction.h"
#include "app/ui/frame_tag_window.h"
#include "app/ui/timeline.h"
#include "base/log.h"
#include "doc/frame_tag.h"

#include <stdexcept>

namespace app {

using namespace doc;

class NewFrameTagCommand : public Command {
public:
  NewFrameTagCommand();
  Command* clone() const override { return new NewFrameTagCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_name;
  int m_from; // 1-based, 0 = use the default range
  int m_to;
  std::string m_color;
  std::string m_aniDir;
};

NewFrameTagCommand::NewFrameTagCommand()
  : Command("NewFrameTag",
            "New Frame Tag",
            CmdRecordableFlag)
  , m_from(0)
  , m_to(0)
{
}

void NewFrameTagCommand::onLoadParams(const Params& params)
{
  m_name = params.get("name");

  std::string from = params.get("from");
  if (!from.empty())
    m_from = strtol(from.c_str(), NULL, 10);
  else
    m_from = 0;

  std::string to = params.get("to");
  if (!to.empty())
    m_to = strtol(to.c_str(), NULL, 10);
  else
    m_to = 0;

  m_color = params.get("color");
  m_aniDir = params.get("ani-dir");
}

std::vector<CommandParamInfo> NewFrameTagCommand::onDescribeParams() const
{
  return {
    {"name", "Name of the new tag. Empty opens the dialog.", false, ""},
    {"from", "1-based first frame of the tag (inclusive). Default: the timeline range, else the current frame.", false, ""},
    {"to", "1-based last frame of the tag (inclusive). Default: the timeline range, else the current frame.", false, ""},
    {"color", "Tag color, for example rgb{255,0,0,255}.", false, ""},
    {"ani-dir", "Animation direction: forward, reverse or mirror.", false, "forward"},
  };
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

  // The from/to params are 1-based, like GotoFrame.
  if (m_from != 0)
    from = MID(0, m_from-1, sprite->lastFrame());
  if (m_to != 0)
    to = MID(0, m_to-1, sprite->lastFrame());

  std::unique_ptr<FrameTag> frameTag(new FrameTag(from, to));

  if (m_name.empty()) {
    // Without a name the tag is created from the dialog, which would
    // block headless.
    if (!App::instance()->isGui()) {
      LOG("NewFrameTag: not available in headless mode\n");
      Console console(context);
      console.printf("NewFrameTag: not available in headless mode\n");
      return;
    }

    FrameTagWindow window(sprite, frameTag.get());
    if (!window.show())
      return;

    window.rangeValue(from, to);
    frameTag->setFrameRange(from, to);
    frameTag->setName(window.nameValue());
    frameTag->setColor(window.colorValue());
    frameTag->setAniDir(window.aniDirValue());
  }
  else {
    frameTag->setName(m_name);

    if (!m_color.empty()) {
      app::Color color = app::Color::fromString(m_color);
      frameTag->setColor(doc::rgba(color.getRed(), color.getGreen(),
                                   color.getBlue(), color.getAlpha()));
    }

    if (m_aniDir == "reverse")
      frameTag->setAniDir(AniDir::REVERSE);
    else if (m_aniDir == "mirror")
      frameTag->setAniDir(AniDir::PING_PONG);
    // "forward" (or an unknown value) keeps the default direction.
  }

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
