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
#include "app/context.h"
#include "app/ini_file.h"
#include "app/launcher.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "app/ui/color_button.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/image.h"
#include "render/render.h"
#include "she/display.h"
#include "she/system.h"
#include "ui/ui.h"

#include "options.xml.h"

namespace app {

static const char* kSectionThemeId = "section_theme";

using namespace ui;

class OptionsWindow : public app::gen::Options {

  class ThemeItem : public ListItem {
  public:
    ThemeItem(const std::string& path,
              const std::string& name)
      : ListItem(name.empty() ? "-- " + path + " --": name),
        m_path(path),
        m_name(name) {
    }

    const std::string& themePath() const { return m_path; }
    const std::string& themeName() const { return m_name; }

    void openFolder() const {
      app::launcher::open_folder(
        m_name.empty() ? m_path: base::join_path(m_path, m_name));
    }

    bool canSelect() const {
      return !m_name.empty();
    }

  private:
    std::string m_path;
    std::string m_name;
  };
public:
  OptionsWindow(Context* context, int& curSection)
    : m_pref(Preferences::instance())
    , m_globPref(m_pref.document(nullptr))
    , m_docPref(m_pref.document(context->activeDocument()))
    , m_curPref(&m_docPref)
    , m_checked_bg_color1(new ColorButton(app::Color::fromMask(), IMAGE_RGB))
    , m_checked_bg_color2(new ColorButton(app::Color::fromMask(), IMAGE_RGB))
    , m_pixelGridColor(new ColorButton(app::Color::fromMask(), IMAGE_RGB))
    , m_gridColor(new ColorButton(app::Color::fromMask(), IMAGE_RGB))
    , m_cursorColor(new ColorButton(m_pref.editor.cursorColor(), IMAGE_RGB))
    , m_curSection(curSection)
  {
    sectionListbox()->Change.connect(base::Bind<void>(&OptionsWindow::onChangeSection, this));

    // Cursor
    cursorColorPlaceholder()->addChild(m_cursorColor);

    if (m_cursorColor->getColor().getType() == app::Color::MaskType) {
      cursorColorType()->setSelectedItemIndex(0);
      m_cursorColor->setVisible(false);
    }
    else {
      cursorColorType()->setSelectedItemIndex(1);
      m_cursorColor->setVisible(true);
    }
    cursorColorType()->Change.connect(base::Bind<void>(&OptionsWindow::onCursorColorType, this));

    // Brush preview
    brushPreview()->setSelectedItemIndex(
      (int)m_pref.editor.brushPreview());

    // Grid color
    m_gridColor->setId("grid_color");
    gridColorPlaceholder()->addChild(m_gridColor);

    // Pixel grid color
    m_pixelGridColor->setId("pixel_grid_color");
    pixelGridColorPlaceholder()->addChild(m_pixelGridColor);

    // Others
    if (m_pref.general.autoshowTimeline())
      autotimeline()->setSelected(true);

    if (m_pref.general.rewindOnStop())
      rewindOnStop()->setSelected(true);

    if (m_pref.general.expandMenubarOnMouseover())
      expandMenubarOnMouseover()->setSelected(true);

    if (m_pref.general.dataRecovery())
      enableDataRecovery()->setSelected(true);

    if (m_pref.general.showFullPath())
      showFullPath()->setSelected(true);

    dataRecoveryPeriod()->setSelectedItemIndex(
      dataRecoveryPeriod()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.dataRecoveryPeriod())));

    if (m_pref.editor.zoomFromCenterWithWheel())
      zoomFromCenterWithWheel()->setSelected(true);

    if (m_pref.editor.zoomFromCenterWithKeys())
      zoomFromCenterWithKeys()->setSelected(true);

    if (m_pref.selection.autoOpaque())
      autoOpaque()->setSelected(true);

    if (m_pref.selection.keepSelectionAfterClear())
      keepSelectionAfterClear()->setSelected(true);

    if (m_pref.experimental.useNativeCursor())
      nativeCursor()->setSelected(true);

    if (m_pref.experimental.useNativeFileDialog())
      nativeFileDialog()->setSelected(true);

    if (m_pref.experimental.flashLayer())
      flashLayer()->setSelected(true);

    if (m_pref.editor.showScrollbars())
      showScrollbars()->setSelected(true);

    // Scope
    gridScope()->addItem("Global");
    if (context->activeDocument()) {
      gridScope()->addItem("Current Document");
      gridScope()->setSelectedItemIndex(1);
      gridScope()->Change.connect(base::Bind<void>(&OptionsWindow::onChangeGridScope, this));
    }

    // Screen/UI Scale
    screenScale()->setSelectedItemIndex(
      screenScale()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.screenScale())));

    uiScale()->setSelectedItemIndex(
      uiScale()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.experimental.uiScale())));

    // Right-click
    rightClickBehavior()->addItem("Paint with background color");
    rightClickBehavior()->addItem("Pick foreground color");
    rightClickBehavior()->addItem("Erase");
    rightClickBehavior()->addItem("Scroll");
    rightClickBehavior()->setSelectedItemIndex((int)m_pref.editor.rightClickMode());

    // Zoom with Scroll Wheel
    wheelZoom()->setSelected(m_pref.editor.zoomWithWheel());

    // Zoom sliding two fingers
#if __APPLE__
    slideZoom()->setSelected(m_pref.editor.zoomWithSlide());
#else
    slideZoom()->setVisible(false);
#endif

    // Checked background size
    checkedBgSize()->addItem("16x16");
    checkedBgSize()->addItem("8x8");
    checkedBgSize()->addItem("4x4");
    checkedBgSize()->addItem("2x2");

    // Checked background colors
    checkedBgColor1Box()->addChild(m_checked_bg_color1);
    checkedBgColor2Box()->addChild(m_checked_bg_color2);

    // Reset button
    reset()->Click.connect(base::Bind<void>(&OptionsWindow::onReset, this));

    // Links
    locateFile()->Click.connect(base::Bind<void>(&OptionsWindow::onLocateConfigFile, this));
#if _WIN32
    locateCrashFolder()->Click.connect(base::Bind<void>(&OptionsWindow::onLocateCrashFolder, this));
#else
    locateCrashFolder()->setVisible(false);
#endif

    // Undo preferences
    undoSizeLimit()->setTextf("%d", m_pref.undo.sizeLimit());
    undoGotoModified()->setSelected(m_pref.undo.gotoModified());
    undoAllowNonlinearHistory()->setSelected(m_pref.undo.allowNonlinearHistory());

    // Theme buttons
    themeList()->Change.connect(base::Bind<void>(&OptionsWindow::onThemeChange, this));
    selectTheme()->Click.connect(base::Bind<void>(&OptionsWindow::onSelectTheme, this));
    openThemeFolder()->Click.connect(base::Bind<void>(&OptionsWindow::onOpenThemeFolder, this));

    // Apply button
    buttonApply()->Click.connect(base::Bind<void>(&OptionsWindow::saveConfig, this));

    onChangeGridScope();
    sectionListbox()->selectIndex(m_curSection);
  }

  bool ok() {
    return (closer() == buttonOk());
  }

  void saveConfig() {
    m_pref.general.autoshowTimeline(autotimeline()->isSelected());
    m_pref.general.rewindOnStop(rewindOnStop()->isSelected());
    m_pref.general.showFullPath(showFullPath()->isSelected());

    bool expandOnMouseover = expandMenubarOnMouseover()->isSelected();
    m_pref.general.expandMenubarOnMouseover(expandOnMouseover);
    ui::MenuBar::setExpandOnMouseover(expandOnMouseover);

    std::string warnings;

    int newPeriod = base::convert_to<int>(dataRecoveryPeriod()->getValue());
    if (enableDataRecovery()->isSelected() != m_pref.general.dataRecovery() ||
        newPeriod != m_pref.general.dataRecoveryPeriod()) {
      m_pref.general.dataRecovery(enableDataRecovery()->isSelected());
      m_pref.general.dataRecoveryPeriod(newPeriod);

      warnings += "<<- Automatically save recovery data every";
    }

    m_pref.editor.zoomFromCenterWithWheel(zoomFromCenterWithWheel()->isSelected());
    m_pref.editor.zoomFromCenterWithKeys(zoomFromCenterWithKeys()->isSelected());
    m_pref.editor.showScrollbars(showScrollbars()->isSelected());
    m_pref.editor.zoomWithWheel(wheelZoom()->isSelected());
#if __APPLE__
    m_pref.editor.zoomWithSlide(slideZoom()->isSelected());
#endif
    m_pref.editor.rightClickMode(static_cast<app::gen::RightClickMode>(rightClickBehavior()->getSelectedItemIndex()));
    m_pref.editor.cursorColor(m_cursorColor->getColor());
    m_pref.editor.brushPreview(static_cast<app::gen::BrushPreview>(brushPreview()->getSelectedItemIndex()));
    m_pref.selection.autoOpaque(autoOpaque()->isSelected());
    m_pref.selection.keepSelectionAfterClear(keepSelectionAfterClear()->isSelected());

    m_curPref->grid.color(m_gridColor->getColor());
    m_curPref->grid.opacity(gridOpacity()->getValue());
    m_curPref->grid.autoOpacity(gridAutoOpacity()->isSelected());
    m_curPref->pixelGrid.color(m_pixelGridColor->getColor());
    m_curPref->pixelGrid.opacity(pixelGridOpacity()->getValue());
    m_curPref->pixelGrid.autoOpacity(pixelGridAutoOpacity()->isSelected());
    m_curPref->bg.type(app::gen::BgType(checkedBgSize()->getSelectedItemIndex()));
    m_curPref->bg.zoom(checkedBgZoom()->isSelected());
    m_curPref->bg.color1(m_checked_bg_color1->getColor());
    m_curPref->bg.color2(m_checked_bg_color2->getColor());

    int undo_size_limit_value;
    undo_size_limit_value = undoSizeLimit()->textInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);

    m_pref.undo.sizeLimit(undo_size_limit_value);
    m_pref.undo.gotoModified(undoGotoModified()->isSelected());
    m_pref.undo.allowNonlinearHistory(undoAllowNonlinearHistory()->isSelected());

    // Experimental features
    m_pref.experimental.useNativeCursor(nativeCursor()->isSelected());
    m_pref.experimental.useNativeFileDialog(nativeFileDialog()->isSelected());
    m_pref.experimental.flashLayer(flashLayer()->isSelected());
    ui::set_use_native_cursors(
      m_pref.experimental.useNativeCursor());

    bool reset_screen = false;
    int newScreenScale = base::convert_to<int>(screenScale()->getValue());
    if (newScreenScale != m_pref.general.screenScale()) {
      m_pref.general.screenScale(newScreenScale);
      reset_screen = true;
    }

    int newUIScale = base::convert_to<int>(uiScale()->getValue());
    if (newUIScale != m_pref.experimental.uiScale()) {
      m_pref.experimental.uiScale(newUIScale);
      warnings += "<<- UI Elements Scale";
    }

    m_pref.save();

    if (!warnings.empty()) {
      ui::Alert::show(PACKAGE
        "<<You must restart the program to see your changes to:%s"
        "||&OK", warnings.c_str());
    }

    if (reset_screen) {
      ui::Manager* manager = ui::Manager::getDefault();
      she::Display* display = manager->getDisplay();
      display->setScale(newScreenScale);
      manager->setDisplay(display);
    }
  }

private:
  void onChangeSection() {
    ListItem* item = static_cast<ListItem*>(sectionListbox()->getSelectedChild());
    if (!item)
      return;

    panel()->showChild(findChild(item->getValue().c_str()));
    m_curSection = sectionListbox()->getSelectedIndex();

    // Load themes
    if (item->getValue() == kSectionThemeId)
      loadThemes();
  }

  void onChangeGridScope() {
    int item = gridScope()->getSelectedItemIndex();

    switch (item) {
      case 0: m_curPref = &m_globPref; break;
      case 1: m_curPref = &m_docPref; break;
    }

    m_gridColor->setColor(m_curPref->grid.color());
    gridOpacity()->setValue(m_curPref->grid.opacity());
    gridAutoOpacity()->setSelected(m_curPref->grid.autoOpacity());

    m_pixelGridColor->setColor(m_curPref->pixelGrid.color());
    pixelGridOpacity()->setValue(m_curPref->pixelGrid.opacity());
    pixelGridAutoOpacity()->setSelected(m_curPref->pixelGrid.autoOpacity());

    checkedBgSize()->setSelectedItemIndex(int(m_curPref->bg.type()));
    checkedBgZoom()->setSelected(m_curPref->bg.zoom());
    m_checked_bg_color1->setColor(m_curPref->bg.color1());
    m_checked_bg_color2->setColor(m_curPref->bg.color2());
  }

  void onReset() {
    // Reset global preferences (use default values specified in pref.xml)
    if (m_curPref == &m_globPref) {
      DocumentPreferences& pref = m_globPref;

      m_gridColor->setColor(pref.grid.color.defaultValue());
      gridOpacity()->setValue(pref.grid.opacity.defaultValue());
      gridAutoOpacity()->setSelected(pref.grid.autoOpacity.defaultValue());

      m_pixelGridColor->setColor(pref.pixelGrid.color.defaultValue());
      pixelGridOpacity()->setValue(pref.pixelGrid.opacity.defaultValue());
      pixelGridAutoOpacity()->setSelected(pref.pixelGrid.autoOpacity.defaultValue());

      checkedBgSize()->setSelectedItemIndex(int(pref.bg.type.defaultValue()));
      checkedBgZoom()->setSelected(pref.bg.zoom.defaultValue());
      m_checked_bg_color1->setColor(pref.bg.color1.defaultValue());
      m_checked_bg_color2->setColor(pref.bg.color2.defaultValue());
    }
    // Reset document preferences with global settings
    else {
      DocumentPreferences& pref = m_globPref;

      m_gridColor->setColor(pref.grid.color());
      gridOpacity()->setValue(pref.grid.opacity());
      gridAutoOpacity()->setSelected(pref.grid.autoOpacity());

      m_pixelGridColor->setColor(pref.pixelGrid.color());
      pixelGridOpacity()->setValue(pref.pixelGrid.opacity());
      pixelGridAutoOpacity()->setSelected(pref.pixelGrid.autoOpacity());

      checkedBgSize()->setSelectedItemIndex(int(pref.bg.type()));
      checkedBgZoom()->setSelected(pref.bg.zoom());
      m_checked_bg_color1->setColor(pref.bg.color1());
      m_checked_bg_color2->setColor(pref.bg.color2());
    }
  }

  void onLocateCrashFolder() {
    app::launcher::open_folder(base::get_file_path(app::memory_dump_filename()));
  }

  void onLocateConfigFile() {
    app::launcher::open_folder(app::main_config_filename());
  }

  void loadThemes() {
    // Themes already loaded
    if (themeList()->getItemsCount() > 0)
      return;

    auto userFolder = userThemeFolder();
    auto folders = themeFolders();
    std::sort(folders.begin(), folders.end());

    for (const auto& path : folders) {
      auto files = base::list_files(path);

      // Only one empty theme folder: the user folder
      if (files.empty() && path != userFolder)
        continue;

      themeList()->addChild(new ThemeItem(path, std::string()));
      std::sort(files.begin(), files.end());
      for (auto& fn : files) {
        if (!base::is_directory(base::join_path(path, fn)))
          continue;

        ThemeItem* item = new ThemeItem(path, fn);
        themeList()->addChild(item);

        // Selected theme
        if (fn == m_pref.theme.selected())
          themeList()->selectChild(item);
      }
    }

    themeList()->layout();
  }

  void onThemeChange() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    selectTheme()->setEnabled(item && item->canSelect());
  }

  void onSelectTheme() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    if (item &&
        item->themeName() != m_pref.theme.selected()) {
      m_pref.theme.selected(item->themeName());

      ui::Alert::show(PACKAGE
                      "<<You must restart the program to see the selected theme"
                      "||&OK");
    }
  }

  void onOpenThemeFolder() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    if (item)
      item->openFolder();
  }

  void onCursorColorType() {
    switch (cursorColorType()->getSelectedItemIndex()) {
      case 0:
        m_cursorColor->setColor(app::Color::fromMask());
        m_cursorColor->setVisible(false);
        break;
      case 1:
        m_cursorColor->setColor(app::Color::fromRgb(0, 0, 0, 255));
        m_cursorColor->setVisible(true);
        break;
    }
    layout();
  }

  static std::string userThemeFolder() {
    ResourceFinder rf;
    rf.includeDataDir("skins");

    // Create user folder to store skins
    try {
      if (!base::is_directory(rf.defaultFilename()))
        base::make_all_directories(rf.defaultFilename());
    }
    catch (...) {
      // Ignore errors
    }

    return base::normalize_path(rf.defaultFilename());
  }

  static std::vector<std::string> themeFolders() {
    ResourceFinder rf;
    rf.includeDataDir("skins");

    std::vector<std::string> paths;
    while (rf.next())
      paths.push_back(base::normalize_path(rf.filename()));
    return paths;
  }

  Preferences& m_pref;
  DocumentPreferences& m_globPref;
  DocumentPreferences& m_docPref;
  DocumentPreferences* m_curPref;
  ColorButton* m_checked_bg_color1;
  ColorButton* m_checked_bg_color2;
  ColorButton* m_pixelGridColor;
  ColorButton* m_gridColor;
  ColorButton* m_cursorColor;
  int& m_curSection;
};

class OptionsCommand : public Command {
public:
  OptionsCommand();
  Command* clone() const override { return new OptionsCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

OptionsCommand::OptionsCommand()
  : Command("Options",
            "Options",
            CmdUIOnlyFlag)
{
  Preferences& preferences = Preferences::instance();

  ui::MenuBar::setExpandOnMouseover(
    preferences.general.expandMenubarOnMouseover());
}

void OptionsCommand::onExecute(Context* context)
{
  static int curSection = 0;

  OptionsWindow window(context, curSection);
  window.openWindowInForeground();
  if (window.ok())
    window.saveConfig();
}

Command* CommandFactory::createOptionsCommand()
{
  return new OptionsCommand;
}

} // namespace app
