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
#include "app/cmd/set_frame_tag_anidir.h"
#include "app/cmd/set_frame_tag_color.h"
#include "app/cmd/set_frame_tag_name.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/loop_tag.h"
#include "app/transaction.h"
#include "app/ui/frame_tag_window.h"
#include "base/convert_to.h"
#include "base/log.h"
#include "doc/anidir.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"

namespace app {

using namespace ui;

class FrameTagPropertiesCommand : public Command {
public:
  FrameTagPropertiesCommand();
  Command* clone() const override { return new FrameTagPropertiesCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_tagName;
  ObjectId m_tagId;
  bool m_hasParams; // true when a change param is given
  std::string m_newName;
  int m_from; // 1-based, 0 = keep the current range
  int m_to;
  std::string m_color;
  std::string m_aniDir;
};

FrameTagPropertiesCommand::FrameTagPropertiesCommand()
  : Command("FrameTagProperties",
            "Frame Tag Properties",
            CmdUIOnlyFlag)
  , m_tagId(NullId)
  , m_hasParams(false)
  , m_from(0)
  , m_to(0)
{
}

void FrameTagPropertiesCommand::onLoadParams(const Params& params)
{
  m_tagName = params.get("name");

  std::string id = params.get("id");
  if (!id.empty())
    m_tagId = ObjectId(base::convert_to<ObjectId>(id));
  else
    m_tagId = NullId;

  m_newName = params.get("new-name");

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

  m_hasParams = !m_newName.empty() || m_from != 0 || m_to != 0 ||
                !m_color.empty() || !m_aniDir.empty();
}

std::vector<CommandParamInfo> FrameTagPropertiesCommand::onDescribeParams() const
{
  return {
    {"name", "Name of the tag to edit. Empty: the tag of the current frame.", false, ""},
    {"id", "Object id of the tag to edit (overrides name).", false, ""},
    {"new-name", "New name for the tag. Skips the dialog.", false, ""},
    {"from", "1-based first frame of the tag (inclusive).", false, ""},
    {"to", "1-based last frame of the tag (inclusive).", false, ""},
    {"color", "Tag color, for example rgb{255,0,0,255}.", false, ""},
    {"ani-dir", "Animation direction: forward, reverse or mirror.", false, "forward"},
  };
}

bool FrameTagPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FrameTagPropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  frame_t frame = reader.frame();
  const FrameTag* foundTag = nullptr;

  if (!m_tagName.empty())
    foundTag = sprite->frameTags().getByName(m_tagName);
  else if (m_tagId != NullId)
    foundTag = sprite->frameTags().getById(m_tagId);
  else
    foundTag = sprite->frameTags().innerTag(frame);

  if (!foundTag)
    return;

  // With the change params, apply them without the dialog.
  if (m_hasParams) {
    ContextWriter writer(reader);
    Transaction transaction(writer.context(), "Change Frame Tag Properties");
    FrameTag* tag = const_cast<FrameTag*>(foundTag);

    if (!m_newName.empty() && tag->name() != m_newName)
      transaction.execute(new cmd::SetFrameTagName(tag, m_newName));

    if (m_from != 0 || m_to != 0) {
      // The from/to params are 1-based, like GotoFrame.
      doc::frame_t from = tag->fromFrame();
      doc::frame_t to = tag->toFrame();
      if (m_from != 0)
        from = MID(0, m_from-1, sprite->lastFrame());
      if (m_to != 0)
        to = MID(0, m_to-1, sprite->lastFrame());
      if (tag->fromFrame() != from || tag->toFrame() != to)
        transaction.execute(new cmd::SetFrameTagRange(tag, from, to));
    }

    if (!m_color.empty()) {
      app::Color color = app::Color::fromString(m_color);
      doc::color_t docColor = doc::rgba(color.getRed(), color.getGreen(),
                                        color.getBlue(), color.getAlpha());
      if (tag->color() != docColor)
        transaction.execute(new cmd::SetFrameTagColor(tag, docColor));
    }

    doc::AniDir anidir = tag->aniDir(); // unknown value: no change
    if (m_aniDir == "forward")
      anidir = doc::AniDir::FORWARD;
    else if (m_aniDir == "reverse")
      anidir = doc::AniDir::REVERSE;
    else if (m_aniDir == "mirror")
      anidir = doc::AniDir::PING_PONG;
    if (tag->aniDir() != anidir)
      transaction.execute(new cmd::SetFrameTagAniDir(tag, anidir));

    transaction.commit();
    return;
  }

  // The dialog would block headless.
  if (!App::instance()->isGui()) {
    LOG("FrameTagProperties: not available in headless mode\n");
    Console console(context);
    console.printf("FrameTagProperties: not available in headless mode\n");
    return;
  }

  FrameTagWindow window(sprite, foundTag);
  if (!window.show())
    return;

  ContextWriter writer(reader);
  Transaction transaction(writer.context(), "Change Frame Tag Properties");
  FrameTag* tag = const_cast<FrameTag*>(foundTag);

  std::string name = window.nameValue();
  if (tag->name() != name)
    transaction.execute(new cmd::SetFrameTagName(tag, name));

  doc::frame_t from, to;
  window.rangeValue(from, to);
  if (tag->fromFrame() != from ||
      tag->toFrame() != to) {
    transaction.execute(new cmd::SetFrameTagRange(tag, from, to));
  }

  doc::color_t docColor = window.colorValue();
  if (tag->color() != docColor)
    transaction.execute(new cmd::SetFrameTagColor(tag, docColor));

  doc::AniDir anidir = window.aniDirValue();
  if (tag->aniDir() != anidir)
    transaction.execute(new cmd::SetFrameTagAniDir(tag, anidir));

  transaction.commit();
}

Command* CommandFactory::createFrameTagPropertiesCommand()
{
  return new FrameTagPropertiesCommand;
}

} // namespace app
