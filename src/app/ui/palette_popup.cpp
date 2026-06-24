// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/palette_popup.h"

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/launcher.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "base/path.h"
#include "doc/palette.h"
#include "ui/alert.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/window.h"

#include "palette_popup.xml.h"

namespace app {

using namespace ui;

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", ClickBehavior::CloseOnClickInOtherWindow)
  , m_popup(new gen::PalettePopup())
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(m_popup);

  m_popup->loadPal()->Click.connect(base::Bind<void>(&PalettePopup::onLoadPal, this));
  m_popup->renamePal()->Click.connect(base::Bind<void>(&PalettePopup::onRenamePal, this));
  m_popup->deletePal()->Click.connect(base::Bind<void>(&PalettePopup::onDeletePal, this));
  m_popup->openFolder()->Click.connect(base::Bind<void>(&PalettePopup::onOpenFolder, this));

  m_popup->view()->attachToView(&m_paletteListBox);

  m_paletteListBox.DoubleClickItem.connect(base::Bind<void>(&PalettePopup::onLoadPal, this));
  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);
}

void PalettePopup::showPopup(const gfx::Rect& bounds)
{
  m_popup->loadPal()->setEnabled(false);
  m_popup->renamePal()->setEnabled(false);
  m_popup->deletePal()->setEnabled(false);
  m_paletteListBox.selectChild(NULL);

  moveWindow(bounds);

  // Setup the hot-region
  setHotRegion(gfx::Region(gfx::Rect(bounds).enlarge(32 * guiscale())));

  openWindow();
}

void PalettePopup::onPalChange(doc::Palette* palette)
{
  const bool hasSelection = (palette != NULL);
  m_popup->loadPal()->setEnabled(
    UIContext::instance()->activeDocument() && hasSelection);
  m_popup->renamePal()->setEnabled(hasSelection);
  m_popup->deletePal()->setEnabled(hasSelection);
}

void PalettePopup::onLoadPal()
{
  doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    CommandsModule::instance()->getCommandByName(CommandId::SetPalette));
  cmd->setPalette(palette);
  UIContext::instance()->executeCommand(cmd);
}

// Small modal dialog asking for a new palette name. Returns an empty string if
// cancelled.
static std::string ask_for_palette_name(const std::string& current)
{
  Window window(Window::WithTitleBar, "Rename Palette");
  Box* box = new VBox();
  Entry* entry = new Entry(256, "%s", current.c_str());
  Box* buttons = new HBox();
  Button* ok = new Button("&OK");
  Button* cancel = new Button("&Cancel");

  entry->setExpansive(true);
  ok->Click.connect(base::Bind<void>(&Window::closeWindow, &window, ok));
  cancel->Click.connect(base::Bind<void>(&Window::closeWindow, &window, cancel));

  buttons->addChild(ok);
  buttons->addChild(cancel);
  box->addChild(entry);
  box->addChild(buttons);
  window.addChild(box);

  window.remapWindow();
  window.centerWindow();
  window.openWindowInForeground();

  if (window.closer() == ok)
    return entry->text();
  return std::string();
}

void PalettePopup::onRenamePal()
{
  doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  std::string fn = palette->filename();
  if (fn.empty())
    return;

  std::string oldTitle = base::get_file_title(fn);
  std::string ext = base::get_file_extension(fn);
  std::string dir = base::get_file_path(fn);

  std::string newTitle = ask_for_palette_name(oldTitle);
  if (newTitle.empty() || newTitle == oldTitle)
    return;

  std::string newFn = base::join_path(dir, newTitle + "." + ext);
  try {
    base::move_file(fn, newFn);
  }
  catch (const std::exception&) {
    Alert::show("Error<<Could not rename the palette file.||&Close");
    return;
  }

  palette->setFilename(newFn);
  if (Widget* item = m_paletteListBox.getSelectedChild())
    item->setText(newTitle);
  m_paletteListBox.layout();
}

void PalettePopup::onDeletePal()
{
  doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  std::string fn = palette->filename();
  std::string name = m_paletteListBox.selectedPaletteName();
  if (fn.empty())
    return;

  if (Alert::show("Delete Palette"
                  "<<Delete \"%s\" permanently?"
                  "||&Delete||&Cancel", name.c_str()) != 1)
    return;

  try {
    if (base::is_file(fn))
      base::delete_file(fn);
  }
  catch (const std::exception&) {
    Alert::show("Error<<Could not delete the palette file.||&Close");
    return;
  }

  // Remove the entry from the list immediately.
  if (Widget* item = m_paletteListBox.getSelectedChild()) {
    m_paletteListBox.removeChild(item);
    item->deferDelete();
  }
  m_paletteListBox.selectChild(NULL);
  onPalChange(NULL);
  if (View* view = View::getView(&m_paletteListBox))
    view->updateView();
  m_paletteListBox.layout();
}

void PalettePopup::onOpenFolder()
{
  inject<ResourcesLoader> loader{"palette"};
  auto paths = loader->resourcesLocation();
  if (paths.empty())
    return;

  const std::string& folder = paths.back();
  if (!base::launcher::open_folder(folder)) {
    // No file manager handled the request: show the path so the user can
    // browse to it (it lives in accessible shared storage).
    Alert::show("Palettes folder<<%s||&OK", folder.c_str());
  }
}

} // namespace app
