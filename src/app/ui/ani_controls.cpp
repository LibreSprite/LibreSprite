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
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/document_range.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/tools/tool.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "base/bind.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace doc;

enum AniAction {
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
  ACTIONS
};

AniControls::AniControls()
  : ButtonSet(5)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  addItem(theme->parts.aniFirst());
  addItem(theme->parts.aniPrevious());
  addItem(theme->parts.aniPlay());
  addItem(theme->parts.aniNext());
  addItem(theme->parts.aniLast());
  ItemChange.connect(base::Bind(&AniControls::onClickButton, this));

  setTriggerOnMouseUp(true);
  setTransparent(true);
  setBgColor(theme->colors.workspace());

  TooltipManager* tooltips = new TooltipManager;
  addChild(tooltips);
  for (int i=0; i<ACTIONS; ++i)
    tooltips->addTooltipFor(getItem(i), getTooltipFor(i), BOTTOM);

  getItem(ACTION_PLAY)->enableFlags(CTRL_RIGHT_CLICK);
}

void AniControls::updateUsingEditor(Editor* editor)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  getItem(ACTION_PLAY)->setIcon(
    (editor && editor->isPlaying() ?
      theme->parts.aniStop():
      theme->parts.aniPlay()));
}

void AniControls::onClickButton()
{
  int item = selectedItem();
  deselectItems();

  Command* cmd = CommandsModule::instance()->getCommandByName(getCommandId(item));
  if (cmd) {
    UIContext::instance()->executeCommand(cmd);
    updateUsingEditor(current_editor);
  }
}

void AniControls::onRightClick(Item* item)
{
  ButtonSet::onRightClick(item);

  if (item == getItem(ACTION_PLAY) && current_editor)
    current_editor->showAnimationSpeedMultiplierPopup(
      Preferences::instance().editor.playOnce, true);
}

const char* AniControls::getCommandId(int index) const
{
  switch (index) {
    case ACTION_FIRST: return CommandId::GotoFirstFrame;
    case ACTION_PREV: return CommandId::GotoPreviousFrame;
    case ACTION_PLAY: return CommandId::PlayAnimation;
    case ACTION_NEXT: return CommandId::GotoNextFrame;
    case ACTION_LAST: return CommandId::GotoLastFrame;
  }
  ASSERT(false);
  return nullptr;
}

std::string AniControls::getTooltipFor(int index) const
{
  std::string tooltip;

  Command* cmd = CommandsModule::instance()->getCommandByName(getCommandId(index));
  if (cmd) {
    tooltip = cmd->friendlyName();

    Key* key = KeyboardShortcuts::instance()->command(cmd->id().c_str());
    if (key && !key->accels().empty()) {
      tooltip += "\n\nShortcut: ";
      tooltip += key->accels().front().toString();
    }
  }

  return tooltip;
}

} // namespace app
