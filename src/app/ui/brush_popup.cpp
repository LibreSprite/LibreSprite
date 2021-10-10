// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/brush_popup.h"

#include "app/app.h"
#include "app/brush_slot.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/context_bar.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/border.h"
#include "gfx/region.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/button.h"
#include "ui/link_label.h"
#include "ui/listitem.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/separator.h"

#include "brush_slot_params.xml.h"

namespace app {

using namespace app::skin;
using namespace doc;
using namespace ui;

namespace {

void show_popup_menu(PopupWindow* popupWindow, Menu* popupMenu,
                     const gfx::Point& pt)
{
  // Here we make the popup window temporaly floating, so it's
  // not closed by the popup menu.
  popupWindow->makeFloating();

  popupMenu->showPopup(pt);

  // Add the menu popup region to the window popup hot region so it's
  // not closed after we close the menu.
  popupWindow->makeFixed();

  gfx::Region rgn;
  rgn.createUnion(gfx::Region(popupWindow->bounds()),
                  gfx::Region(popupMenu->bounds()));
  popupWindow->setHotRegion(rgn);
}

class SelectBrushItem : public ButtonSet::Item {
protected:
  SelectBrushItem() = default;

public:
  const BrushSlot& brush() const {
    return *m_brush;
  }

  static std::shared_ptr<SelectBrushItem> create(const BrushSlot& brush, int slot = -1) {
    std::shared_ptr<SelectBrushItem> item = inject<Widget>{"SelectBrushItem"};
    item->m_brush = &brush;
    if (brush.hasBrush()) {
      SkinPartPtr icon(new SkinPart);
      icon->setBitmap(0, BrushPopup::createSurfaceForBrush(brush.brush()));
      item->setIcon(icon);
    }
    return item;
  }

private:
  void onClick() override {
    ContextBar* contextBar = App::instance()->contextBar();

    if (m_slot >= 0)
      contextBar->setActiveBrushBySlot(m_slot);
    else if (m_brush->hasBrush()) {
      tools::Tool* tool = App::instance()->activeTool();
      auto& brushPref = Preferences::instance().tool(tool).brush;
      BrushRef brush;

      brush.reset(
        new Brush(
            static_cast<doc::BrushType>(m_brush->brush()->type()),
            brushPref.size(),
            brushPref.angle()));

      contextBar->setActiveBrush(brush);
    }
  }

  const BrushSlot* m_brush = nullptr;
  int m_slot;
};

static ui::Widget::Shared<SelectBrushItem> _sbi("SelectBrushItem");

class BrushShortcutItem : public ButtonSet::Item {
public:
  static std::shared_ptr<BrushShortcutItem> create(const std::string& text, int slot) {
    std::shared_ptr<BrushShortcutItem> item = inject<Widget>{"BrushShortcutItem"};
    item->m_slot = slot;
    item->setText(text);
    return item;
  }

private:
  void onClick() override {
    Params params;
    params.set("change", "custom");
    params.set("slot", std::to_string(m_slot));
    Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::ChangeBrush);
    cmd->loadParams(params);
    std::string search = cmd->friendlyName();
    if (!search.empty()) {
      params.clear();
      params.set("search", search);
      cmd = CommandsModule::instance()->getCommandByName(CommandId::KeyboardShortcuts);
      ASSERT(cmd);
      if (cmd)
        UIContext::instance()->executeCommand(cmd, params);
    }
  }

  int m_slot;
};

static ui::Widget::Shared<BrushShortcutItem> _bsi("BrushShortcutItem");

class BrushOptionsItem : public ButtonSet::Item {
public:
  static std::shared_ptr<BrushOptionsItem> create(BrushPopup* popup, int slot) {
    std::shared_ptr<BrushOptionsItem> item = inject<Widget>{"BrushOptionsItem"};
    item->m_popup = popup;
    item->m_slot = slot;
    item->setIcon(SkinTheme::instance()->parts.iconArrowDown(), true);
    return item;
  }

private:

  void onClick() override {
    Menu menu;
    AppMenuItem save("Save Brush Here");
    AppMenuItem lockItem("Locked");
    AppMenuItem deleteItem("Delete");
    AppMenuItem deleteAllItem("Delete All");

    lockItem.setSelected(m_brushes.isBrushSlotLocked(m_slot));

    save.Click.connect(&BrushOptionsItem::onSaveBrush, this);
    lockItem.Click.connect(&BrushOptionsItem::onLockBrush, this);
    deleteItem.Click.connect(&BrushOptionsItem::onDeleteBrush, this);
    deleteAllItem.Click.connect(&BrushOptionsItem::onDeleteAllBrushes, this);

    menu.addChild(&save);
    menu.addChild(new MenuSeparator);
    menu.addChild(&lockItem);
    menu.addChild(&deleteItem);
    menu.addChild(new MenuSeparator);
    menu.addChild(&deleteAllItem);
    menu.addChild(new Label(""));
    menu.addChild(new Separator("Saved Parameters", HORIZONTAL));

    app::gen::BrushSlotParams params;
    menu.addChild(&params);

    // Load preferences
    BrushSlot brush = m_brushes.getBrushSlot(m_slot);
    params.brushType()->setSelected(brush.hasFlag(BrushSlot::Flags::BrushType));
    params.brushSize()->setSelected(brush.hasFlag(BrushSlot::Flags::BrushSize));
    params.brushAngle()->setSelected(brush.hasFlag(BrushSlot::Flags::BrushAngle));
    params.fgColor()->setSelected(brush.hasFlag(BrushSlot::Flags::FgColor));
    params.bgColor()->setSelected(brush.hasFlag(BrushSlot::Flags::BgColor));
    params.inkType()->setSelected(brush.hasFlag(BrushSlot::Flags::InkType));
    params.inkOpacity()->setSelected(brush.hasFlag(BrushSlot::Flags::InkOpacity));
    params.shade()->setSelected(brush.hasFlag(BrushSlot::Flags::Shade));
    params.pixelPerfect()->setSelected(brush.hasFlag(BrushSlot::Flags::PixelPerfect));

    m_changeFlags = true;
    show_popup_menu(m_popup, &menu,
                    gfx::Point(origin().x, origin().y+bounds().h));

    if (m_changeFlags) {
      brush = m_brushes.getBrushSlot(m_slot);

      int flags = (int(brush.flags()) & int(BrushSlot::Flags::Locked));
      if (params.brushType()->isSelected()) flags |= int(BrushSlot::Flags::BrushType);
      if (params.brushSize()->isSelected()) flags |= int(BrushSlot::Flags::BrushSize);
      if (params.brushAngle()->isSelected()) flags |= int(BrushSlot::Flags::BrushAngle);
      if (params.fgColor()->isSelected()) flags |= int(BrushSlot::Flags::FgColor);
      if (params.bgColor()->isSelected()) flags |= int(BrushSlot::Flags::BgColor);
      if (params.inkType()->isSelected()) flags |= int(BrushSlot::Flags::InkType);
      if (params.inkOpacity()->isSelected()) flags |= int(BrushSlot::Flags::InkOpacity);
      if (params.shade()->isSelected()) flags |= int(BrushSlot::Flags::Shade);
      if (params.pixelPerfect()->isSelected()) flags |= int(BrushSlot::Flags::PixelPerfect);

      if (brush.flags() != BrushSlot::Flags(flags)) {
        brush.setFlags(BrushSlot::Flags(flags));
        m_brushes.setBrushSlot(m_slot, brush);
      }
    }
  }

private:

  void onSaveBrush() {
    ContextBar* contextBar = App::instance()->contextBar();

    m_brushes.setBrushSlot(
      m_slot, contextBar->createBrushSlotFromPreferences());
    m_brushes.lockBrushSlot(m_slot);

    m_changeFlags = false;
  }

  void onLockBrush() {
    if (m_brushes.isBrushSlotLocked(m_slot))
      m_brushes.unlockBrushSlot(m_slot);
    else
      m_brushes.lockBrushSlot(m_slot);
  }

  void onDeleteBrush() {
    m_brushes.removeBrushSlot(m_slot);
    m_changeFlags = false;
  }

  void onDeleteAllBrushes() {
    m_brushes.removeAllBrushSlots();
    m_changeFlags = false;
  }

  BrushPopup* m_popup;
  AppBrushes& m_brushes = App::instance()->brushes();
  BrushRef m_brush;
  int m_slot;
  bool m_changeFlags;
};

static ui::Widget::Shared<BrushOptionsItem> _boi("BrushOptionsItem");


class NewCustomBrushItem : public ButtonSet::Item {
public:
  NewCustomBrushItem() {
    setText("Save Brush");
  }

private:
  void onClick() override {
    ContextBar* contextBar = App::instance()->contextBar();

    auto& brushes = App::instance()->brushes();
    int slot = brushes.addBrushSlot(
      contextBar->createBrushSlotFromPreferences());
    brushes.lockBrushSlot(slot);
  }
};

static ui::Widget::Shared<NewCustomBrushItem> _ncbi("NewCustomBrushItem");

class NewBrushOptionsItem : public ButtonSet::Item {
public:
  NewBrushOptionsItem() {
    setIcon(SkinTheme::instance()->parts.iconArrowDown(), true);
  }

private:
  void onClick() override {
    Menu menu;

    menu.addChild(new Separator("Parameters to Save", HORIZONTAL));

    app::gen::BrushSlotParams params;
    menu.addChild(&params);

    // Load preferences
    auto& saveBrush = Preferences::instance().saveBrush;
    params.brushType()->setSelected(saveBrush.brushType());
    params.brushSize()->setSelected(saveBrush.brushSize());
    params.brushAngle()->setSelected(saveBrush.brushAngle());
    params.fgColor()->setSelected(saveBrush.fgColor());
    params.bgColor()->setSelected(saveBrush.bgColor());
    params.inkType()->setSelected(saveBrush.inkType());
    params.inkOpacity()->setSelected(saveBrush.inkOpacity());
    params.shade()->setSelected(saveBrush.shade());
    params.pixelPerfect()->setSelected(saveBrush.pixelPerfect());

    show_popup_menu(static_cast<PopupWindow*>(window()), &menu,
                    gfx::Point(origin().x, origin().y+bounds().h));

    // Save preferences
    if (saveBrush.brushType() != params.brushType()->isSelected())
      saveBrush.brushType(params.brushType()->isSelected());
    if (saveBrush.brushSize() != params.brushSize()->isSelected())
      saveBrush.brushSize(params.brushSize()->isSelected());
    if (saveBrush.brushAngle() != params.brushAngle()->isSelected())
      saveBrush.brushAngle(params.brushAngle()->isSelected());
    if (saveBrush.fgColor() != params.fgColor()->isSelected())
      saveBrush.fgColor(params.fgColor()->isSelected());
    if (saveBrush.bgColor() != params.bgColor()->isSelected())
      saveBrush.bgColor(params.bgColor()->isSelected());
    if (saveBrush.inkType() != params.inkType()->isSelected())
      saveBrush.inkType(params.inkType()->isSelected());
    if (saveBrush.inkOpacity() != params.inkOpacity()->isSelected())
      saveBrush.inkOpacity(params.inkOpacity()->isSelected());
    if (saveBrush.shade() != params.shade()->isSelected())
      saveBrush.shade(params.shade()->isSelected());
    if (saveBrush.pixelPerfect() != params.pixelPerfect()->isSelected())
      saveBrush.pixelPerfect(params.pixelPerfect()->isSelected());
  }
};

static ui::Widget::Shared<NewBrushOptionsItem> _nboi("NewBrushOptionsItem");

} // anonymous namespace

BrushPopup::BrushPopup() : PopupWindow("", ClickBehavior::CloseOnClickInOtherWindow) {
  m_standardBrushes->setColumns(3);
  auto& brushes = App::instance()->brushes();

  setAutoRemap(false);
  setBorder(gfx::Border(2)*guiscale());
  setChildSpacing(0);
  m_box.noBorderNoChildSpacing();
  m_standardBrushes->setTriggerOnMouseUp(true);

  addChild(&m_box);

  HBox* top = new HBox;
  top->addChild(m_standardBrushes);
  top->addChild(new BoxFiller);

  m_box.addChild(top);
  m_box.addChild(new Separator("", HORIZONTAL));

  for (const auto& brush : brushes.getStandardBrushes()) {
    m_standardBrushes->addItem(SelectBrushItem::create({BrushSlot::Flags::BrushType, brush}));
  }

  m_standardBrushes->setTransparent(true);
  m_standardBrushes->setBgColor(gfx::ColorNone);

  brushes.ItemsChange.connect(&BrushPopup::onBrushChanges, this);
}

void BrushPopup::setBrush(Brush* brush)
{
  for (auto child : m_standardBrushes->children()) {
    auto item = std::static_pointer_cast<SelectBrushItem>(child->shared_from_this());
    // Same type and same image
    if (item->brush().hasBrush() &&
        item->brush().brush()->type() == brush->type() &&
        (brush->type() != kImageBrushType ||
         item->brush().brush()->image() == brush->image())) {
      m_standardBrushes->setSelectedItem(item);
      return;
    }
  }
}

void BrushPopup::regenerate(const gfx::Rect& box)
{
  auto& brushSlots = App::instance()->brushes().getBrushSlots();

  if (m_customBrushes) {
    // As BrushPopup::regenerate() can be called when a
    // "m_customBrushes" button is clicked we cannot delete
    // "m_customBrushes" right now.
    m_customBrushes->parent()->removeChild(m_customBrushes);
    m_customBrushes->deferDelete();
  }

  m_customBrushes = inject<Widget>{"ButtonSet"};
  m_customBrushes->setColumns(3);
  m_customBrushes->setTriggerOnMouseUp(true);

  int slot = 0;
  for (const auto& brush : brushSlots) {
    ++slot;

    // Get shortcut
    std::string shortcut;
    {
      Params params;
      params.set("change", "custom");
      params.set("slot", base::convert_to<std::string>(slot).c_str());
      Key* key = KeyboardShortcuts::instance()->command(
        CommandId::ChangeBrush, params);
      if (key && !key->accels().empty())
        shortcut = key->accels().front().toString();
    }
    m_customBrushes->addItem(SelectBrushItem::create(brush, slot));
    m_customBrushes->addItem(BrushShortcutItem::create(shortcut, slot));
    m_customBrushes->addItem(BrushOptionsItem::create(this, slot));
  }

  m_customBrushes->addItem(inject<Widget>{"NewCustomBrushItem"}.shared<ButtonSet::Item>(), 2, 1);
  m_customBrushes->addItem(inject<Widget>{"NewBrushOptionsItem"}.shared<ButtonSet::Item>());
  m_customBrushes->setExpansive(true);
  m_box.addChild(m_customBrushes);

  // Resize the window and change the hot region.
  setBounds(gfx::Rect(box.origin(), sizeHint()));
  setHotRegion(gfx::Region(bounds()));
}

void BrushPopup::onBrushChanges()
{
  if (isVisible()) {
    gfx::Region rgn;
    getDrawableRegion(rgn, DrawableRegionFlags(kCutTopWindows | kUseChildArea));

    regenerate(bounds());
    invalidate();

    parent()->invalidateRegion(rgn);
  }
}

// static
she::Surface* BrushPopup::createSurfaceForBrush(const BrushRef& origBrush)
{
  Image* image = nullptr;
  BrushRef brush = origBrush;
  if (brush) {
    if (brush->type() != kImageBrushType && brush->size() > 10) {
      brush.reset(new Brush(*brush));
      brush->setSize(10);
    }
    image = brush->image();
  }

  she::Surface* surface = she::instance()->createRgbaSurface(
    std::min(10, image ? image->width(): 4),
    std::min(10, image ? image->height(): 4));

  if (image) {
    Palette* palette = get_current_palette();
    if (image->pixelFormat() == IMAGE_BITMAP) {
      palette = new Palette(frame_t(0), 2);
      palette->setEntry(0, rgba(0, 0, 0, 0));
      palette->setEntry(1, rgba(0, 0, 0, 255));
    }

    convert_image_to_surface(
      image, palette, surface,
      0, 0, 0, 0, image->width(), image->height());

    if (image->pixelFormat() == IMAGE_BITMAP)
      delete palette;
  }
  else {
    surface->clear();
  }

  return surface;
}

} // namespace app
