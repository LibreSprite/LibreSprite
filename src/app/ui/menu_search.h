// LibreSprite
// Copyright (C) 2024 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/params.h"
#include "ui/box.h"
#include "ui/listbox.h"
#include "ui/popup_window.h"

#include <string>
#include <vector>

namespace app {

  class Command;
  class Key;
  class SearchEntry;

  class MenuSearch : public ui::HBox {
  public:
    struct CommandEntry {
      Command* command;
      Params params;
      std::string displayName;
      std::string searchText;  // lowercase for matching
      Key* shortcut;
    };

    struct PreferenceEntry {
      std::string settingName;    // "Grid Color"
      std::string sectionName;    // "Grid & Background"
      std::string sectionId;      // "grid" (for Options command param)
      std::string displayName;    // "Edit > Preferences > Grid & Background > Grid Color"
      std::string searchText;     // lowercase keywords for matching
    };

    MenuSearch();
    ~MenuSearch();

    void focusSearch();

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:

    void onSearchChange();
    void buildCommandIndex();
    void buildPreferenceIndex();
    void filterCommands(const std::string& query);
    void showResults();
    void hideResults();
    void executeSelected();
    void selectNext();
    void selectPrevious();

    SearchEntry* m_searchEntry;
    ui::PopupWindow* m_popup;
    ui::ListBox* m_resultsList;
    std::vector<CommandEntry> m_allCommands;
    std::vector<CommandEntry*> m_filteredCommands;
    std::vector<PreferenceEntry> m_allPreferences;
    std::vector<PreferenceEntry*> m_filteredPreferences;
  };

} // namespace app
