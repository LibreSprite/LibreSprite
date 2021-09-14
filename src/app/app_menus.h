// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <unordered_map>
#include "base/connection.h"
#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "recent_files_menu.h"
#include "script/script_menu.h"
#include "ui/base.h"
#include "ui/menu.h"

class TiXmlElement;
class TiXmlHandle;

namespace app {
  class Key;
  class Command;
  class Params;

  using namespace ui;

  // Class to handle/get/reload available menus in gui.xml file.
  class AppMenus {
    AppMenus() = default;
    DISABLE_COPYING(AppMenus);

  public:
    static AppMenus* instance();

    void reload();
    void rebuildRecentList();
    void rebuildScriptsList();

    template <typename Type=Menu>
    Type* getById(const std::string& id) {
        auto iterator = m_identifiedWidgets.find(id);
        if (iterator == m_identifiedWidgets.end())
            return nullptr;
        return dynamic_cast<Type*>(iterator->second);
    }

    Menu* getRootMenu() { return getById("main_menu"); }
    Menu* getTabPopupMenu() { return getById("tab_popup"); }
    Menu* getDocumentTabPopupMenu() { return getById("document_tab_popup"); }
    Menu* getLayerPopupMenu() { return getById("layer_popup"); }
    Menu* getFramePopupMenu() { return getById("frame_popup"); }
    Menu* getCelPopupMenu() { return getById("cel_popup"); }
    Menu* getCelMovementPopupMenu() { return getById("cel_movement_popup"); }
    Menu* getFrameTagPopupMenu() { return getById("frame_tag_popup"); }
    Menu* getPalettePopupMenu() { return getById("palette_popup"); }
    Menu* getInkPopupMenu() { return getById("ink_popup"); }

    void applyShortcutToMenuitemsWithCommand(Command* command, const Params& params, Key* key);

  private:
    void loadMenus(TiXmlHandle& handle);
    Menu* convertXmlelemToMenu(TiXmlElement* elem);
    Widget* convertXmlelemToMenuitem(TiXmlElement* elem);
    Widget* createInvalidVersionMenuitem();
    void applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, const Params& params, Key* key);
    void clearIdentifiedWidgets();
    RecentFilesMenu m_recentFilesMenu;
    ScriptMenu m_scriptMenu;
    std::unordered_map<std::string, Widget*> m_identifiedWidgets;
  };

} // namespace app
