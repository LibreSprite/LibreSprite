// LibreSprite
// Copyright (C) 2024 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/menu_search.h"

#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/search_entry.h"
#include "ui/entry.h"
#include "app/ui_context.h"
#include "base/string.h"
#include <cstdio>
#include "ui/graphics.h"
#include "ui/label.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"

// Set to true to enable debug logging
constexpr bool MENU_SEARCH_DEBUG = false;
#include "ui/message.h"
#include "ui/size_hint_event.h"

#include <algorithm>
#include <cctype>

namespace app {

using namespace ui;

namespace {

std::string toLower(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
    [](unsigned char c) { return std::tolower(c); });
  return result;
}

// Remove '&' mnemonic markers from menu text
std::string cleanMenuText(const std::string& text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    if (c != '&') {
      result += c;
    }
  }
  return result;
}

// Calculate edit distance between two strings (for fuzzy matching)
int editDistance(const std::string& a, const std::string& b) {
  size_t m = a.size();
  size_t n = b.size();

  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

  for (size_t i = 0; i <= m; i++) dp[i][0] = i;
  for (size_t j = 0; j <= n; j++) dp[0][j] = j;

  for (size_t i = 1; i <= m; i++) {
    for (size_t j = 1; j <= n; j++) {
      if (a[i-1] == b[j-1]) {
        dp[i][j] = dp[i-1][j-1];
      } else {
        dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
      }
    }
  }
  return dp[m][n];
}

// Check if query fuzzy-matches any word in the text
bool fuzzyMatchWord(const std::string& query, const std::string& text) {
  // Split text into words and check each
  std::string word;
  for (size_t i = 0; i <= text.size(); i++) {
    char c = (i < text.size()) ? text[i] : ' ';
    if (c == ' ' || c == '>' || c == '-' || c == '_') {
      if (!word.empty()) {
        // Allow edit distance of 2 (covers transpositions like gird->grid)
        // Be more strict for very short queries
        int maxDist = (query.size() <= 2) ? 1 : 2;
        if (editDistance(query, word) <= maxDist) {
          return true;
        }
        word.clear();
      }
    } else {
      word += c;
    }
  }
  return false;
}

} // anonymous namespace

class MenuSearchResultItem : public ListItem {
public:
  MenuSearchResultItem(MenuSearch::CommandEntry* entry, bool enabled)
    : ListItem("")
    , m_entry(entry)
    , m_commandEnabled(enabled) {
    std::string text = entry->displayName;
    if (entry->shortcut && !entry->shortcut->accels().empty()) {
      text += "  [" + entry->shortcut->accels().front().toString() + "]";
    }
    if (!enabled) {
      text += " (disabled)";
    }
    setText(text);
    // Don't call setEnabled - we want selection to work, just visual difference
  }

  MenuSearch::CommandEntry* entry() const { return m_entry; }
  bool isCommandEnabled() const { return m_commandEnabled; }

protected:
  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    gfx::Rect bounds = clientBounds();
    gfx::Color fg, bg;

    if (isSelected()) {
      if (m_commandEnabled) {
        // Selected + enabled: bright blue background
        bg = gfx::rgba(60, 120, 200);
        fg = gfx::rgba(255, 255, 255);
      } else {
        // Selected + disabled: dimmer blue background
        bg = gfx::rgba(80, 80, 120);
        fg = gfx::rgba(180, 180, 180);
      }
    } else {
      if (m_commandEnabled) {
        // Normal + enabled
        bg = gfx::rgba(45, 45, 45);
        fg = gfx::rgba(220, 220, 220);
      } else {
        // Normal + disabled
        bg = gfx::rgba(45, 45, 45);
        fg = gfx::rgba(100, 100, 100);
      }
    }

    g->fillRect(bg, bounds);

    if (hasText()) {
      bounds.shrink(border());
      g->drawString(text(), fg, gfx::ColorNone,
                    gfx::Point(bounds.x, bounds.y + bounds.h/2 - textHeight()/2));
    }
  }

private:
  MenuSearch::CommandEntry* m_entry;
  bool m_commandEnabled;
};

// Custom SearchEntry that forwards navigation keys
class MenuSearchEntry : public SearchEntry {
public:
  base::Signal0<void> NavigateUp;
  base::Signal0<void> NavigateDown;
  base::Signal0<void> Execute;
  base::Signal0<void> Cancel;

protected:
  bool onProcessMessage(Message* msg) override {
    if (msg->type() == kKeyDownMessage) {
      KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
      KeyScancode scancode = keyMsg->scancode();

      if (scancode == kKeyDown) {
        NavigateDown();
        return true;
      }
      else if (scancode == kKeyUp) {
        NavigateUp();
        return true;
      }
      else if (scancode == kKeyEnter || scancode == kKeyEnterPad) {
        Execute();
        return true;
      }
      else if (scancode == kKeyEsc) {
        Cancel();
        return true;
      }
    }
    return Entry::onProcessMessage(msg);
  }
};

MenuSearch::MenuSearch()
  : HBox()
  , m_popup(nullptr)
  , m_resultsList(nullptr)
{
  auto* entry = new MenuSearchEntry();
  m_searchEntry = entry;
  m_searchEntry->setMinSize(gfx::Size(300, 0));
  m_searchEntry->setExpansive(false);

  // Show shortcut hint as placeholder
#ifdef __APPLE__
  m_searchEntry->setSuffix("Cmd+Shift+P");
#else
  m_searchEntry->setSuffix("Ctrl+Shift+P");
#endif

  addChild(m_searchEntry);

  m_searchEntry->Change.connect([this]{ onSearchChange(); });
  entry->NavigateDown.connect([this]{ selectNext(); });
  entry->NavigateUp.connect([this]{ selectPrevious(); });
  entry->Execute.connect([this]{ executeSelected(); });
  entry->Cancel.connect([this]{
    hideResults();
    m_searchEntry->setText("");
#ifdef __APPLE__
    m_searchEntry->setSuffix("Cmd+Shift+P");
#else
    m_searchEntry->setSuffix("Ctrl+Shift+P");
#endif
    m_searchEntry->releaseFocus();
  });

  buildCommandIndex();
}

MenuSearch::~MenuSearch()
{
  hideResults();
}

void MenuSearch::focusSearch()
{
  m_searchEntry->requestFocus();
}

bool MenuSearch::onProcessMessage(Message* msg)
{
  if (msg->type() == kKeyDownMessage) {
    KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
    KeyScancode scancode = keyMsg->scancode();
    KeyModifiers modifiers = keyMsg->modifiers();

    // Cmd+Shift+P (Mac) or Ctrl+Shift+P (others)
    bool cmdOrCtrl = (modifiers & kKeyCtrlModifier) || (modifiers & kKeyCmdModifier);
    bool shift = (modifiers & kKeyShiftModifier);

    if (scancode == kKeyP && cmdOrCtrl && shift) {
      focusSearch();
      return true;
    }
  }
  return HBox::onProcessMessage(msg);
}

void MenuSearch::buildCommandIndex()
{
  m_allCommands.clear();

  // Helper to recursively traverse menus
  std::function<void(Menu*, const std::string&)> traverseMenu;
  traverseMenu = [&](Menu* menu, const std::string& prefix) {
    if (!menu) return;

    for (Widget* child : menu->children()) {
      if (MenuItem* menuItem = dynamic_cast<MenuItem*>(child)) {
        std::string itemText = cleanMenuText(menuItem->text());
        std::string fullPath = prefix.empty() ? itemText : prefix + " > " + itemText;

        // Check if it's an AppMenuItem with a command
        if (AppMenuItem* appItem = dynamic_cast<AppMenuItem*>(menuItem)) {
          Command* cmd = appItem->getCommand();
          if (cmd) {
            CommandEntry entry;
            entry.command = cmd;
            entry.params = appItem->getParams();
            entry.displayName = fullPath;
            entry.searchText = toLower(fullPath + " " + cmd->friendlyName());
            entry.shortcut = appItem->key();
            m_allCommands.push_back(entry);
          }
        }

        // Recurse into submenus
        if (menuItem->hasSubmenu()) {
          traverseMenu(menuItem->getSubmenu(), fullPath);
        }
      }
    }
  };

  Menu* rootMenu = AppMenus::instance()->getRootMenu();
  if (rootMenu) {
    traverseMenu(rootMenu, "");
  }
}

void MenuSearch::onSearchChange()
{
  std::string query = m_searchEntry->text();

  // Show/hide shortcut hint based on whether there's text
  if (query.empty()) {
#ifdef __APPLE__
    m_searchEntry->setSuffix("Cmd+Shift+P");
#else
    m_searchEntry->setSuffix("Ctrl+Shift+P");
#endif
    hideResults();
    return;
  } else {
    m_searchEntry->setSuffix("");
  }

  filterCommands(query);
  showResults();
}

void MenuSearch::filterCommands(const std::string& query)
{
  m_filteredCommands.clear();

  std::string lowerQuery = toLower(query);

  // Split query into words for multi-word matching
  std::vector<std::string> queryWords;
  std::string word;
  for (char c : lowerQuery) {
    if (c == ' ') {
      if (!word.empty()) {
        queryWords.push_back(word);
        word.clear();
      }
    } else {
      word += c;
    }
  }
  if (!word.empty()) {
    queryWords.push_back(word);
  }

  // First pass: exact substring matching
  for (auto& entry : m_allCommands) {
    bool matches = true;
    for (const auto& qw : queryWords) {
      if (entry.searchText.find(qw) == std::string::npos) {
        matches = false;
        break;
      }
    }

    if (matches) {
      m_filteredCommands.push_back(&entry);
    }
  }

  // Second pass: fuzzy matching if no exact matches found
  if (m_filteredCommands.empty()) {
    for (auto& entry : m_allCommands) {
      bool matches = true;
      for (const auto& qw : queryWords) {
        // Try fuzzy match for each query word
        if (!fuzzyMatchWord(qw, entry.searchText)) {
          matches = false;
          break;
        }
      }

      if (matches) {
        m_filteredCommands.push_back(&entry);
      }
    }
  }

  // Sort by display name length (shorter = better match)
  std::sort(m_filteredCommands.begin(), m_filteredCommands.end(),
    [](const CommandEntry* a, const CommandEntry* b) {
      return a->displayName.length() < b->displayName.length();
    });

  // Limit results
  if (m_filteredCommands.size() > 15) {
    m_filteredCommands.resize(15);
  }
}

void MenuSearch::showResults()
{
  if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: showResults, %d filtered commands\n", (int)m_filteredCommands.size());
  if (m_filteredCommands.empty()) {
    hideResults();
    return;
  }

  // Close existing popup and create fresh one each time
  if (m_popup) {
    m_popup->closeWindow(nullptr);
    delete m_popup;
    m_popup = nullptr;
    m_resultsList = nullptr;
  }

  m_popup = new PopupWindow("", PopupWindow::ClickBehavior::DoNothingOnClick,
                             PopupWindow::EnterBehavior::DoNothingOnEnter);

  auto* vbox = new VBox();
  m_resultsList = new ListBox();
  m_resultsList->setExpansive(true);
  vbox->addChild(m_resultsList);

  auto* hint = new Label("[Arrow keys to navigate, Enter to select]");
  vbox->addChild(hint);

  m_popup->addChild(vbox);

  m_resultsList->DoubleClickItem.connect([this]{
    executeSelected();
  });


  UIContext* ctx = UIContext::instance();
  for (auto* entry : m_filteredCommands) {
    bool enabled = entry->command && ctx && entry->command->isEnabled(ctx);
    m_resultsList->addChild(new MenuSearchResultItem(entry, enabled));
  }

  // Select first item
  if (m_resultsList->firstChild()) {
    m_resultsList->selectChild(m_resultsList->firstChild());
  }

  // Position popup below search entry
  gfx::Rect searchBounds = m_searchEntry->bounds();
  int itemHeight = 22;
  gfx::Size popupSize(std::max(searchBounds.w, 400),
                      std::min((int)m_filteredCommands.size() * itemHeight + 8, 350));

  gfx::Rect popupBounds(searchBounds.x, searchBounds.y + searchBounds.h,
                        popupSize.w, popupSize.h);
  m_popup->setBounds(popupBounds);

  // Enlarge hot region for easier clicking
  gfx::Region hotRegion(gfx::Rect(popupBounds).enlarge(16));
  hotRegion |= gfx::Region(gfx::Rect(searchBounds).enlarge(8));
  m_popup->setHotRegion(hotRegion);

  m_popup->openWindow();
  m_popup->layout();
  m_popup->invalidate();
}

void MenuSearch::hideResults()
{
  if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: hideResults called\n");
  if (m_popup && m_popup->isVisible()) {
    m_popup->closeWindow(nullptr);
  }
}

void MenuSearch::executeSelected()
{
  if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: executeSelected called\n");
  if (!m_resultsList) {
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: no results list\n");
    return;
  }

  Widget* selected = m_resultsList->getSelectedChild();
  if (!selected) {
    // Try to get first item if nothing selected
    selected = m_resultsList->firstChild();
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: no selection, trying first child\n");
  }
  if (!selected) {
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: no items\n");
    return;
  }

  MenuSearchResultItem* item = dynamic_cast<MenuSearchResultItem*>(selected);
  if (!item || !item->entry()) {
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: invalid item\n");
    return;
  }

  CommandEntry* entry = item->entry();
  if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: executing %s\n", entry->displayName.c_str());

  hideResults();
  m_searchEntry->setText("");
#ifdef __APPLE__
  m_searchEntry->setSuffix("Cmd+Shift+P");
#else
  m_searchEntry->setSuffix("Ctrl+Shift+P");
#endif
  m_searchEntry->releaseFocus();

  if (entry->command) {
    UIContext* context = UIContext::instance();
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: context=%p, command=%s\n", (void*)context, entry->command->id().c_str());
    if (context && entry->command->isEnabled(context)) {
      if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: command is enabled, executing...\n");
      entry->command->loadParams(entry->params);
      context->executeCommand(entry->command, entry->params);
      if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: executeCommand returned\n");
    } else {
      if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: command not enabled (context=%p, isEnabled=%d)\n",
              (void*)context, context ? entry->command->isEnabled(context) : -1);
    }
  } else {
    if (MENU_SEARCH_DEBUG) fprintf(stderr, "MenuSearch: no command pointer!\n");
  }
}

void MenuSearch::selectNext()
{
  if (!m_resultsList) return;

  int index = m_resultsList->getSelectedIndex();
  int count = (int)m_resultsList->getItemsCount();

  if (index < count - 1) {
    m_resultsList->selectIndex(index + 1);
  }
}

void MenuSearch::selectPrevious()
{
  if (!m_resultsList) return;

  int index = m_resultsList->getSelectedIndex();

  if (index > 0) {
    m_resultsList->selectIndex(index - 1);
  }
}

} // namespace app
