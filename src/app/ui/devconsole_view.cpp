// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/devconsole_view.h"
#include "app/app_menus.h"
#include "app/script/app_scripting.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "script/engine.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/message.h"
#include "ui/system.h"

namespace app {

using namespace ui;
using namespace app::skin;

class DevConsoleView::CommmandEntry : public Entry {
public:
  CommmandEntry() : Entry(256, "") {
    setFocusStop(true);
    setFocusMagnet(true);
  }

  base::Signal1<void, const std::string&> ExecuteCommand;

protected:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kKeyDownMessage:
        if (hasFocus()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          KeyScancode scancode = keymsg->scancode();

          switch (scancode) {
            case kKeyEnter:
            case kKeyEnterPad: {
              std::string cmd = text();
              ExecuteCommand(cmd);
              setText("");
              return true;
            }
          }
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }
};

DevConsoleView::DevConsoleView()
  : Box(VERTICAL)
  , m_textBox("Welcome to LibreSprite Scripting Console\n(Experimental)", LEFT)
  , m_label(">")
  , m_entry(new CommmandEntry)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  addChild(&m_view);
  addChild(&m_bottomBox);

  m_bottomBox.addChild(&m_language);
  m_bottomBox.addChild(&m_label);
  m_bottomBox.addChild(m_entry);

  auto& engines = script::Engine::getRegistry();
  for (auto& entry : engines) {
      std::string name = entry.first;
      if (!name.empty())
        m_language.addItem(name);
  }

  m_view.setProperty(SkinStylePropertyPtr(
      new SkinStyleProperty(theme->styles.workspaceView())));

  m_view.attachToView(&m_textBox);
  m_view.setExpansive(true);
  m_entry->setExpansive(true);
  m_entry->ExecuteCommand.connect(&DevConsoleView::onExecuteCommand, this);
}

DevConsoleView::~DevConsoleView()
{
  // m_document->removeObserver(this);
  // delete m_editor;
}

std::string DevConsoleView::getTabText()
{
  return "Console";
}

TabIcon DevConsoleView::getTabIcon()
{
  return TabIcon::NONE;
}

WorkspaceView* DevConsoleView::cloneWorkspaceView()
{
  return new DevConsoleView();
}

void DevConsoleView::onWorkspaceViewSelected()
{
  m_entry->requestFocus();
}

bool DevConsoleView::onCloseView(Workspace* workspace, bool quitting)
{
  workspace->removeView(this);
  return true;
}

void DevConsoleView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getTabPopupMenu();
  if (!menu)
    return;

  menu->showPopup(ui::get_mouse_position());
}

bool DevConsoleView::onProcessMessage(Message* msg)
{
  return Box::onProcessMessage(msg);
}

void DevConsoleView::onExecuteCommand(const std::string& cmd)
{
  script::Engine::setDefault(m_language.getValue());
  m_engine.printLastResult();
  m_engine.eval(cmd);
}

void DevConsoleView::onConsolePrint(const char* text)
{
  if (text)
    m_textBox.setText(m_textBox.text() + "\n" + text);
}

} // namespace app
