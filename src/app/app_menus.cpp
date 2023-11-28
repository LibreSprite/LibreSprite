// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_menus.h"
#include "app/file_system.h"

#include "base/string.h"
#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/gui_xml.h"
#include "app/resource_finder.h"
#include "app/tools/tool_box.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/util/filetoks.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "ui/ui.h"

#include "tinyxml2.h"

#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

static void destroy_instance(AppMenus* instance)
{
  delete instance;
}

// static
AppMenus* AppMenus::instance()
{
  static AppMenus* instance = NULL;
  if (!instance) {
    instance = new AppMenus;
    App::instance()->Exit.connect(base::Bind<void>(&destroy_instance, instance));
  }
  return instance;
}

void AppMenus::reload()
{
  XmlDocumentRef doc(GuiXml::instance()->doc());
  tinyxml2::XMLHandle handle(doc.get());
  const char* path = GuiXml::instance()->filename();

  ////////////////////////////////////////
  // Load menus

  LOG(" - Loading menus from \"%s\"...\n", path);

  loadMenus(handle);
  

#if _DEBUG
  // Add a warning element because the user is not using the last well-known gui.xml file.
  if (GuiXml::instance()->version() != VERSION)
    getRootMenu()->insertChild(0, createInvalidVersionMenuitem());
#endif

  LOG("Main menu loaded.\n");

  rebuildScriptsList();

  ////////////////////////////////////////
  // Load keyboard shortcuts for commands

  LOG(" - Loading commands keyboard shortcuts from \"%s\"...\n", path);

  tinyxml2::XMLElement* xmlKey = handle
    .FirstChildElement("gui")
    .FirstChildElement("keyboard").ToElement();

  KeyboardShortcuts::instance()->clear();
  KeyboardShortcuts::instance()->importFile(xmlKey, KeySource::Original);

  // Load user settings
  {
    ResourceFinder rf;
    rf.includeUserDir("user.aseprite-keys");
    std::string fn = rf.getFirstOrCreateDefault();
    if (base::is_file(fn))
      KeyboardShortcuts::instance()->importFile(fn, KeySource::UserDefined);
  }
}

void AppMenus::rebuildRecentList() {
    m_recentFilesMenu.rebuildRecentList();
}

void AppMenus::clearIdentifiedWidgets() {
    for (auto entry : m_identifiedWidgets) {
        if (auto parent = entry.second->parent()) {
            parent->removeChild(entry.second);
        }
    }
    for (auto entry : m_identifiedWidgets) {
        delete entry.second;
    }
    m_identifiedWidgets.clear();
}

void AppMenus::rebuildScriptsList() {
  m_scriptMenu.rebuildScriptsList(getById("script_list"));
}

void AppMenus::loadMenus(tinyxml2::XMLHandle& handle)
{
    clearIdentifiedWidgets();

    // <gui><menus><menu>
    tinyxml2::XMLElement* xmlMenu = handle
        .FirstChildElement("gui")
        .FirstChildElement("menus")
        .FirstChildElement("menu").ToElement();
    for (; xmlMenu; xmlMenu = xmlMenu->NextSiblingElement()) {
        auto menu = convertXmlelemToMenu(xmlMenu);
        if ( menu->id().empty()) {
            delete menu;
        }
    }
}

Menu* AppMenus::convertXmlelemToMenu(tinyxml2::XMLElement* elem)
{
  Menu* menu = new Menu();

  //LOG("convertXmlelemToMenu(%s, %s, %s)\n", elem->Value(), elem->Attribute("id"), elem->Attribute("text"));

  auto id = elem->Attribute("id");
  if (id) {
      menu->setId(id);
      m_identifiedWidgets[id] = menu;
  }

  tinyxml2::XMLElement* child = elem->FirstChildElement();
  while (child) {
    Widget* menuitem = convertXmlelemToMenuitem(child);
    if (menuitem)
      menu->addChild(menuitem);
    else
      throw base::Exception("Error converting the element \"%s\" to a menu-item.\n",
                            static_cast<const char*>(child->Value()));

    child = child->NextSiblingElement();
  }

  return menu;
}

Widget* AppMenus::convertXmlelemToMenuitem(tinyxml2::XMLElement* elem)
{
  // is it a <separator>?
  if (strcmp(elem->Value(), "separator") == 0)
    return new MenuSeparator;

  const char* command_name = elem->Attribute("command");
  Command* command =
    command_name ? CommandsModule::instance()->getCommandByName(command_name):
                   NULL;

  // load params
  Params params;
  if (command) {
    tinyxml2::XMLElement* xmlParam = elem->FirstChildElement("param");
    while (xmlParam) {
      const char* param_name = xmlParam->Attribute("name");
      const char* param_value = xmlParam->Attribute("value");

      if (param_name && param_value)
        params.set(param_name, param_value);

      xmlParam = xmlParam->NextSiblingElement();
    }
  }

  // Create the item
  AppMenuItem* menuitem = new AppMenuItem(elem->Attribute("text"), command, params);
  if (!menuitem)
    return NULL;

  /* has it a ID? */
  
  if (const char* id = elem->Attribute("id")) {
    m_identifiedWidgets[id] = menuitem;
  }

  // Has it a sub-menu (<menu>)?
  if (strcmp(elem->Value(), "menu") == 0) {
    // Create the sub-menu
    Menu* subMenu = convertXmlelemToMenu(elem);
    if (!subMenu)
      throw base::Exception("Error reading the sub-menu\n");

    menuitem->setSubmenu(subMenu);
  }

  return menuitem;
}

Widget* AppMenus::createInvalidVersionMenuitem()
{
  AppMenuItem* menuitem = new AppMenuItem("WARNING!");
  Menu* subMenu = new Menu();
  subMenu->addChild(new AppMenuItem(PACKAGE " is using a customized gui.xml (maybe from your HOME directory)."));
  subMenu->addChild(new AppMenuItem("You should update your customized gui.xml file to the new version to get"));
  subMenu->addChild(new AppMenuItem("the latest commands available."));
  subMenu->addChild(new MenuSeparator);
  subMenu->addChild(new AppMenuItem("You can bypass this validation adding the correct version"));
  subMenu->addChild(new AppMenuItem("number in <gui version=\"" VERSION "\"> element."));
  menuitem->setSubmenu(subMenu);
  return menuitem;
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Command* command, const Params& params, Key* key)
{
  // TODO redesign the list of popup menus, it might be an
  //      autogenerated widget from 'gen'
  Menu* menus[] = {
    getRootMenu(),
    getTabPopupMenu(),
    getDocumentTabPopupMenu(),
    getLayerPopupMenu(),
    getFramePopupMenu(),
    getCelPopupMenu(),
    getCelMovementPopupMenu(),
    getFrameTagPopupMenu(),
    getPalettePopupMenu(),
    getInkPopupMenu(),
    getById("script_list")
  };

  for (Menu* menu : menus)
    if (menu)
      applyShortcutToMenuitemsWithCommand(menu, command, params, key);
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, const Params& params, Key* key)
{
  for (auto child : menu->children()) {
    if (child->type() == kMenuItemWidget) {
      AppMenuItem* menuitem = dynamic_cast<AppMenuItem*>(child);
      if (!menuitem)
        continue;

      Command* mi_command = menuitem->getCommand();
      const Params& mi_params = menuitem->getParams();

      if ((mi_command) &&
          (base::utf8_icmp(mi_command->id(), command->id()) == 0) &&
          (mi_params == params)) {
        // Set the keyboard shortcut to be shown in this menu-item
        menuitem->setKey(key);
      }

      if (Menu* submenu = menuitem->getSubmenu())
        applyShortcutToMenuitemsWithCommand(submenu, command, params, key);
    }
  }
}

} // namespace app
