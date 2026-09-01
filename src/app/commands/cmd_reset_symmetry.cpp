// LibreSprite
// Copyright (C) 2026 LibreSprite contributors
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
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "doc/sprite.h"
#include "ui/view.h"

namespace app {

class ResetSymmetryCommand : public Command {
public:
  ResetSymmetryCommand();
  Command* clone() const override { return new ResetSymmetryCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  enum class Target { Center, ViewCenter };

  Target m_target;
};

ResetSymmetryCommand::ResetSymmetryCommand()
  : Command("ResetSymmetry",
            "Reset Symmetry",
            CmdUIOnlyFlag)
  , m_target(Target::Center)
{
}

void ResetSymmetryCommand::onLoadParams(const Params& params)
{
  m_target = (params.get("target") == "view" ? Target::ViewCenter : Target::Center);
}

bool ResetSymmetryCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ResetSymmetryCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* doc = reader.document();
  const Sprite* sprite = reader.sprite();

  gfx::Point pos(sprite->width()/2, sprite->height()/2);

  if (m_target == Target::ViewCenter && current_editor) {
    pos = current_editor->screenToEditor(
      ui::View::getView(current_editor)->viewportBounds().center());
    pos.x = MID(0, pos.x, sprite->width());
    pos.y = MID(0, pos.y, sprite->height());
  }

  auto& symmetry = Preferences::instance().document(doc).symmetry;
  symmetry.xAxis(pos.x);
  symmetry.yAxis(pos.y);

  if (current_editor)
    current_editor->invalidate();
}

Command* CommandFactory::createResetSymmetryCommand()
{
  return new ResetSymmetryCommand;
}

} // namespace app
