// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/context_bar.h"

#include "app/app.h"
#include "app/app_brushes.h"
#include "app/app_menus.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/document.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/tools/active_tool.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/brush_popup.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/icon_button.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "base/unique_ptr.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/listitem.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/popup_window.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

static bool g_updatingFromCode = false;

class ContextBar::BrushTypeField : public ButtonSet {
public:
  BrushTypeField(ContextBar* owner) : m_owner(owner) {
    setColumns(1);

    auto part = std::make_shared<SkinPart>();
    part->setBitmap(0, BrushPopup::createSurfaceForBrush(BrushRef(nullptr)));
    addItem(part);
  }

  ~BrushTypeField() {
    closePopup();
  }

  void updateBrush(tools::Tool* tool = nullptr) {
    SkinPartPtr part(new SkinPart);
    part->setBitmap(
      0, BrushPopup::createSurfaceForBrush(
        m_owner->activeBrush(tool)));

    getItem(0)->setIcon(part);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    m_popupWindow.setupTooltips(tooltipManager);
  }

  void showPopupAndHighlightSlot(int slot) {
    openPopup();
  }

protected:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    if (!m_popupWindow.isVisible())
      openPopup();
    else
      closePopup();
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ev.setSizeHint(Size(16, 18)*guiscale());
  }

private:
  // Returns a little rectangle that can be used by the popup as the
  // first brush position.
  gfx::Rect getPopupBox() {
    Rect rc = bounds();
    rc.y += rc.h - 2*guiscale();
    rc.setSize(sizeHint());
    return rc;
  }

  void openPopup() {
    doc::BrushRef brush = m_owner->activeBrush();

    m_popupWindow.regenerate(getPopupBox());
    m_popupWindow.setBrush(brush.get());

    Region rgn(m_popupWindow.bounds().createUnion(bounds()));
    m_popupWindow.setHotRegion(rgn);

    m_popupWindow.openWindow();
  }

  void closePopup() {
    m_popupWindow.closeWindow(NULL);
    deselectItems();
  }

  ContextBar* m_owner;
  AppBrushes& m_brushes = App::instance()->brushes();
  BrushPopup m_popupWindow;
};

class ContextBar::BrushSizeField : public IntEntry {
public:
  BrushSizeField() : IntEntry(Brush::kMinBrushSize, Brush::kMaxBrushSize) {
    setSuffix("px");
  }

private:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.size(getValue());
  }
};

class ContextBar::BrushAngleField : public IntEntry
{
public:
  BrushAngleField(BrushTypeField* brushType)
    : IntEntry(0, 180)
    , m_brushType(brushType) {
    setSuffix("\xc2\xb0");
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.angle(getValue());

    m_brushType->updateBrush();
  }

private:
  BrushTypeField* m_brushType;
};

class ContextBar::BrushPatternField : public ComboBox
{
public:
  BrushPatternField() : m_lock(false) {
    addItem("Pattern aligned to source");
    addItem("Pattern aligned to destination");
    addItem("Paint brush");
  }

  void setBrushPattern(BrushPattern type) {
    int index = 0;

    switch (type) {
      case BrushPattern::ALIGNED_TO_SRC: index = 0; break;
      case BrushPattern::ALIGNED_TO_DST: index = 1; break;
      case BrushPattern::PAINT_BRUSH: index = 2; break;
    }

    m_lock = true;
    setSelectedItemIndex(index);
    m_lock = false;
  }

protected:
  void onChange() override {
    ComboBox::onChange();

    if (m_lock)
      return;

    BrushPattern type = BrushPattern::ALIGNED_TO_SRC;

    switch (getSelectedItemIndex()) {
      case 0: type = BrushPattern::ALIGNED_TO_SRC; break;
      case 1: type = BrushPattern::ALIGNED_TO_DST; break;
      case 2: type = BrushPattern::PAINT_BRUSH; break;
    }

    Preferences::instance().brush.pattern(type);
  }

  bool m_lock;
};

class ContextBar::ToleranceField : public IntEntry
{
public:
  ToleranceField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).tolerance(getValue());
  }
};

class ContextBar::ContiguousField : public CheckBox
{
public:
  ContiguousField() : CheckBox("Contiguous") {
    setup_mini_font(this);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).contiguous(isSelected());

    releaseFocus();
  }
};

class ContextBar::PaintBucketSettingsField : public ButtonSet {
public:
  PaintBucketSettingsField() {
    setColumns(1);
    addItem(SkinTheme::instance()->parts.timelineGear());
  }

protected:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);
    const gfx::Rect bounds = this->bounds();

    Tool* tool = App::instance()->activeTool();
    auto& toolPref = Preferences::instance().tool(tool);

    Menu menu;
    MenuItem
      stopAtGrid("Stop at Grid"),
      activeLayer("Refer only active layer"),
      allLayers("Refer visible layers");
    menu.addChild(&stopAtGrid);
    menu.addChild(new MenuSeparator());
    menu.addChild(&activeLayer);
    menu.addChild(&allLayers);

    stopAtGrid.setSelected(
      toolPref.floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE);
    activeLayer.setSelected(
      toolPref.floodfill.referTo() == app::gen::FillReferTo::ACTIVE_LAYER);
    allLayers.setSelected(
      toolPref.floodfill.referTo() == app::gen::FillReferTo::ALL_LAYERS);

    stopAtGrid.Click.connect(
      [&]{
        toolPref.floodfill.stopAtGrid(
          toolPref.floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE ?
          app::gen::StopAtGrid::NEVER: app::gen::StopAtGrid::IF_VISIBLE);
      });
    activeLayer.Click.connect(
      [&]{
        toolPref.floodfill.referTo(app::gen::FillReferTo::ACTIVE_LAYER);
      });
    allLayers.Click.connect(
      [&]{
        toolPref.floodfill.referTo(app::gen::FillReferTo::ALL_LAYERS);
      });

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
    deselectItems();
  }

};

class ContextBar::InkTypeField : public ButtonSet {
public:
  InkTypeField(ContextBar* owner) : m_owner(owner) {
    setColumns(1);
    addItem(SkinTheme::instance()->parts.inkSimple());
  }

  void setInkType(InkType inkType) {
    Preferences& pref = Preferences::instance();

    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->toolBox())
        pref.tool(tool).ink(inkType);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).ink(inkType);
    }

    m_owner->updateForActiveTool();
  }

  void setInkTypeIcon(InkType inkType) {
    SkinTheme* theme = SkinTheme::instance();
    SkinPartPtr part = theme->parts.inkSimple();

    switch (inkType) {
      case InkType::SIMPLE:            part = theme->parts.inkSimple(); break;
      case InkType::ALPHA_COMPOSITING: part = theme->parts.inkAlphaCompositing(); break;
      case InkType::COPY_COLOR:        part = theme->parts.inkCopyColor(); break;
      case InkType::LOCK_ALPHA:        part = theme->parts.inkLockAlpha(); break;
      case InkType::SHADING:           part = theme->parts.inkShading(); break;
    }

    getItem(0)->setIcon(part);
  }

protected:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    gfx::Rect bounds = this->bounds();

    AppMenus::instance()
      ->getInkPopupMenu()
      ->showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));

    deselectItems();
  }

  ContextBar* m_owner;
};

class ContextBar::InkShadesField : public HBox {

  class ShadeWidget : public Widget {
  public:
    enum ClickType { DragAndDrop, Select };

    base::Signal0<void> Click;

    ShadeWidget(const Shade& colors, ClickType click)
      : Widget(kGenericWidget)
      , m_click(click)
      , m_shade(colors)
      , m_hotIndex(-1)
      , m_dragIndex(-1)
      , m_boxSize(12) {
      setText("Select colors in the palette");
    }

    void reverseShadeColors() {
      std::reverse(m_shade.begin(), m_shade.end());
      invalidate();
    }

    doc::Remap* createShadeRemap(bool left) {
      base::UniquePtr<doc::Remap> remap;
      Shade colors = getShade();

      // We need two or more colors to create a shading remap. In
      // other case, the ShadingInkProcessing will use the full
      // color palette.
      if (colors.size() > 1) {
        remap.reset(new doc::Remap(get_current_palette()->size()));

        for (int i=0; i<remap->size(); ++i)
          remap->map(i, i);

        if (left) {
          for (int i=1; i<int(colors.size()); ++i)
            remap->map(colors[i].getIndex(), colors[i-1].getIndex());
        }
        else {
          for (int i=0; i<int(colors.size())-1; ++i)
            remap->map(colors[i].getIndex(), colors[i+1].getIndex());
        }
      }

      return remap.release();
    }

    int size() const {
      int colors = 0;
      for (const auto& color : m_shade) {
        if ((color.getIndex() >= 0 &&
             color.getIndex() < get_current_palette()->size()) ||
            (m_click == Select)) {
          ++colors;
        }
      }
      return colors;
    }

    Shade getShade() const {
      Shade colors;
      for (const auto& color : m_shade) {
        if ((color.getIndex() >= 0 &&
             color.getIndex() < get_current_palette()->size()) ||
            (m_click == Select)) {
          colors.push_back(color);
        }
      }
      return colors;
    }

    void setShade(const Shade& shade) {
      m_shade = shade;
      invalidate();
      parent()->parent()->layout();
    }

    void updateShadeFromColorBarPicks() {
      auto colorBar = ColorBar::instance();
      if (!colorBar)
        return;

      doc::PalettePicks picks;
      colorBar->getPaletteView()->getSelectedEntries(picks);
      if (picks.picks() >= 2)
        onChangeColorBarSelection();
    }

  private:

    void onChangeColorBarSelection() {
      if (!isVisible())
        return;

      doc::PalettePicks picks;
      ColorBar::instance()->getPaletteView()->getSelectedEntries(picks);

      m_shade.resize(picks.picks());

      int i = 0, j = 0;
      for (bool pick : picks) {
        if (pick)
          m_shade[j++] = app::Color::fromIndex(i);
        ++i;
      }

      parent()->parent()->layout();
    }

    bool onProcessMessage(ui::Message* msg) override {
      switch (msg->type()) {

        case kOpenMessage:
          if (m_click == DragAndDrop) {
            m_conn = ColorBar::instance()->ChangeSelection.connect(
              base::Bind<void>(&ShadeWidget::onChangeColorBarSelection, this));
          }
          break;

        case kSetCursorMessage:
          if (hasCapture()) {
            ui::set_mouse_cursor(kMoveCursor);
            return true;
          }
          break;

        case kMouseEnterMessage:
        case kMouseLeaveMessage:
          invalidate();
          break;

        case kMouseDownMessage: {
          if (m_click == DragAndDrop) {
            if (m_hotIndex >= 0 &&
                m_hotIndex < int(m_shade.size())) {
              m_dragIndex = m_hotIndex;
              m_dropBefore = false;
              captureMouse();
            }
          }
          break;
        }

        case kMouseUpMessage: {
          if (m_click == Select) {
            setSelected(true);
            Click();
            closeWindow();
          }

          if (m_dragIndex >= 0) {
            ASSERT(m_dragIndex < int(m_shade.size()));

            auto color = m_shade[m_dragIndex];
            m_shade.erase(m_shade.begin()+m_dragIndex);
            if (m_hotIndex >= 0)
              m_shade.insert(m_shade.begin()+m_hotIndex, color);

            m_dragIndex = -1;
            invalidate();

            // Relayout the context bar if we have removed an entry.
            if (m_hotIndex < 0)
              parent()->parent()->layout();
          }

          if (hasCapture())
            releaseMouse();
          break;
        }

        case kMouseMoveMessage: {
          MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
          gfx::Point mousePos = mouseMsg->position() - bounds().origin();
          gfx::Rect bounds = clientBounds();
          int hot = -1;

          bounds.shrink(3*guiscale());

          if (bounds.contains(mousePos)) {
            int count = size();
            hot = (mousePos.x - bounds.x) / (m_boxSize*guiscale());
            hot = MID(0, hot, count-1);
          }

          if (m_hotIndex != hot) {
            m_hotIndex = hot;
            invalidate();
          }

          bool dropBefore =
            (hot >= 0 && mousePos.x < (bounds.x+m_boxSize*guiscale()*hot)+m_boxSize*guiscale()/2);
          if (m_dropBefore != dropBefore) {
            m_dropBefore = dropBefore;
            invalidate();
          }
          break;
        }
      }
      return Widget::onProcessMessage(msg);
    }

    void onSizeHint(SizeHintEvent& ev) override {
      int size = this->size();
      if (size < 2)
        ev.setSizeHint(Size((16+m_boxSize)*guiscale()+textWidth(), 18*guiscale()));
      else {
        if (m_click == Select && size > 16)
          size = 16;
        ev.setSizeHint(Size(6+m_boxSize*size, 18)*guiscale());
      }
    }

    void onPaint(PaintEvent& ev) override {
      SkinTheme* theme = SkinTheme::instance();
      Graphics* g = ev.graphics();
      gfx::Rect bounds = clientBounds();

      gfx::Color bg = bgColor();
      if (m_click == Select && hasMouseOver())
        bg = theme->colors.menuitemHighlightFace();
      g->fillRect(bg, bounds);

      Shade colors = getShade();
      if (colors.size() >= 2) {
        int w = (6+m_boxSize*colors.size())*guiscale();
        if (bounds.w > w)
          bounds.w = w;
      }

      skin::Style::State state;
      if (hasMouseOver()) state += Style::hover();
      theme->styles.view()->paint(g, bounds, nullptr, state);

      bounds.shrink(3*guiscale());

      gfx::Rect box(bounds.x, bounds.y, m_boxSize*guiscale(), bounds.h);

      if (colors.size() >= 2) {
        gfx::Rect hotBounds;

        int j = 0;
        for (int i=0; i<int(colors.size()); ++i) {
          if (i == int(colors.size())-1)
            box.w = bounds.x+bounds.w-box.x;

          app::Color color;

          if (m_dragIndex >= 0 &&
              m_hotIndex == i) {
            color = colors[m_dragIndex];
          }
          else {
            if (j == m_dragIndex) {
              ++j;
            }
            if (j < int(colors.size()))
              color = colors[j++];
            else
              color = app::Color::fromMask();
          }

          draw_color(g, box, color,
                     (doc::ColorMode)app_get_current_pixel_format());

          if (m_hotIndex == i)
            hotBounds = box;

          box.x += box.w;
        }

        if (!hotBounds.isEmpty() && m_click == DragAndDrop) {
          hotBounds.enlarge(3*guiscale());

          Style::State state = Style::active();
          state += Style::hover();
          theme->styles.timelineRangeOutline()->paint(
            g, hotBounds, NULL, state);
        }
      }
      else {
        g->fillRect(theme->colors.editorFace(), bounds);
        g->drawAlignedUIString(text(), theme->colors.face(), gfx::ColorNone, bounds,
                               ui::CENTER | ui::MIDDLE);
      }
    }

    ClickType m_click;
    Shade m_shade;
    int m_hotIndex;
    int m_dragIndex;
    bool m_dropBefore;
    int m_boxSize;
    base::ScopedConnection m_conn;
  };

public:
  InkShadesField()
    : m_button(SkinTheme::instance()->parts.iconArrowDown()->bitmap(0))
    , m_shade(Shade(), ShadeWidget::DragAndDrop)
    , m_loaded(false) {
    SkinTheme* theme = SkinTheme::instance();
    m_shade.setBgColor(theme->colors.workspace());
    m_button.setBgColor(theme->colors.workspace());

    noBorderNoChildSpacing();
    addChild(&m_button);
    addChild(&m_shade);

    m_button.setFocusStop(false);
    m_button.Click.connect(base::Bind<void>(&InkShadesField::onShowMenu, this));
  }

  ~InkShadesField() {
    saveShades();
  }

  void reverseShadeColors() {
    m_shade.reverseShadeColors();
  }

  doc::Remap* createShadeRemap(bool left) {
    return m_shade.createShadeRemap(left);
  }

  Shade getShade() const {
    return m_shade.getShade();
  }

  void setShade(const Shade& shade) {
    m_shade.setShade(shade);
  }

  void updateShadeFromColorBarPicks() {
    m_shade.updateShadeFromColorBarPicks();
  }

private:
  void onShowMenu() {
    loadShades();
    gfx::Rect bounds = m_button.bounds();

    Menu menu;
    MenuItem
      reverse("Reverse Shade"),
      save("Save Shade");
    menu.addChild(&reverse);
    menu.addChild(&save);

    bool hasShade = (m_shade.size() >= 2);
    reverse.setEnabled(hasShade);
    save.setEnabled(hasShade);
    reverse.Click.connect(base::Bind<void>(&InkShadesField::reverseShadeColors, this));
    save.Click.connect(base::Bind<void>(&InkShadesField::onSaveShade, this));

    if (!m_shades.empty()) {
      SkinTheme* theme = SkinTheme::instance();

      menu.addChild(new MenuSeparator);

      int i = 0;
      for (const Shade& shade : m_shades) {
        auto shadeWidget = new ShadeWidget(shade, ShadeWidget::Select);
        shadeWidget->setExpansive(true);
        shadeWidget->setBgColor(theme->colors.menuitemNormalFace());
        shadeWidget->Click.connect(
          [&]{
            m_shade.setShade(shade);
          });

        auto close = new IconButton(theme->parts.iconClose()->bitmap(0));
        close->setBgColor(theme->colors.menuitemNormalFace());
        close->Click.connect(
          base::Bind<void>(
            [this, i, close]{
              m_shades.erase(m_shades.begin()+i);
              close->closeWindow();
            }));

        auto item = new HBox();
        item->noBorderNoChildSpacing();
        item->addChild(shadeWidget);
        item->addChild(close);
        menu.addChild(item);
        ++i;
      }
    }

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
    m_button.invalidate();
  }

  void onSaveShade() {
    loadShades();
    m_shades.push_back(m_shade.getShade());
  }

  void loadShades() {
    if (m_loaded)
      return;

    m_loaded = true;

    char buf[32];
    int n = get_config_int("shades", "count", 0);
    n = MID(0, n, 256);
    for (int i=0; i<n; ++i) {
      sprintf(buf, "shade%d", i);
      Shade shade = shade_from_string(get_config_string("shades", buf, ""));
      if (shade.size() >= 2)
        m_shades.push_back(shade);
    }
  }

  void saveShades() {
    if (!m_loaded)
      return;

    char buf[32];
    int n = int(m_shades.size());
    set_config_int("shades", "count", n);
    for (int i=0; i<n; ++i) {
      sprintf(buf, "shade%d", i);
      set_config_string("shades", buf, shade_to_string(m_shades[i]).c_str());
    }
  }

  IconButton m_button;
  ShadeWidget m_shade;
  std::vector<Shade> m_shades;
  bool m_loaded;
};

class ContextBar::InkOpacityField : public IntEntry
{
public:
  InkOpacityField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    int newValue = getValue();
    Preferences& pref = Preferences::instance();
    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->toolBox())
        pref.tool(tool).opacity(newValue);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).opacity(newValue);
    }
  }
};

class ContextBar::SprayWidthField : public IntEntry
{
public:
  SprayWidthField() : IntEntry(1, 32) {
  }

protected:
  void onValueChange() override {
    IntEntry::onValueChange();
    if (g_updatingFromCode)
      return;

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).spray.width(getValue());
  }
};

class ContextBar::SpraySpeedField : public IntEntry
{
public:
  SpraySpeedField() : IntEntry(1, 100) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).spray.speed(getValue());
  }
};

class ContextBar::TransparentColorField : public HBox {
public:
  TransparentColorField(ContextBar* owner) : m_maskColor(app::Color::fromMask(), IMAGE_RGB), m_owner(owner) {
    m_icon->setColumns(1);
    SkinTheme* theme = SkinTheme::instance();

    addChild(m_icon);
    addChild(&m_maskColor);

    m_icon->addItem(theme->parts.selectionOpaque());
    gfx::Size sz = m_icon->getItem(0)->sizeHint();
    sz.w += 2*guiscale();
    m_icon->getItem(0)->setMinSize(sz);

    m_icon->ItemChange.connect(base::Bind<void>(&TransparentColorField::onPopup, this));
    m_maskColor.Change.connect(base::Bind<void>(&TransparentColorField::onChangeColor, this));

    Preferences::instance().selection.opaque.AfterChange.connect(
      base::Bind<void>(&TransparentColorField::onOpaqueChange, this));

    onOpaqueChange();
  }

private:

  void onPopup() {
    gfx::Rect bounds = this->bounds();

    Menu menu;
    MenuItem
      opaque("Opaque"),
      masked("Transparent"),
      automatic("Adjust automatically depending on layer type");
    menu.addChild(&opaque);
    menu.addChild(&masked);
    menu.addChild(new MenuSeparator);
    menu.addChild(&automatic);

    if (Preferences::instance().selection.opaque())
      opaque.setSelected(true);
    else
      masked.setSelected(true);
    automatic.setSelected(Preferences::instance().selection.autoOpaque());

    opaque.Click.connect(base::Bind<void>(&TransparentColorField::setOpaque, this, true));
    masked.Click.connect(base::Bind<void>(&TransparentColorField::setOpaque, this, false));
    automatic.Click.connect(base::Bind<void>(&TransparentColorField::onAutomatic, this));

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
  }

  void onChangeColor() {
    Preferences::instance().selection.transparentColor(
      m_maskColor.getColor());
  }

  void setOpaque(bool opaque) {
    Preferences::instance().selection.opaque(opaque);
  }

  // When the preference is changed from outside the context bar
  void onOpaqueChange() {
    bool opaque = Preferences::instance().selection.opaque();

    SkinTheme* theme = SkinTheme::instance();
    SkinPartPtr part = (opaque ? theme->parts.selectionOpaque():
                                 theme->parts.selectionMasked());
    m_icon->getItem(0)->setIcon(part);

    m_maskColor.setVisible(!opaque);
    if (!opaque) {
      Preferences::instance().selection.transparentColor(
        m_maskColor.getColor());
    }

    if (m_owner)
      m_owner->layout();
  }

  void onAutomatic() {
    Preferences::instance().selection.autoOpaque(
      !Preferences::instance().selection.autoOpaque());
  }

  std::shared_ptr<ButtonSet> m_icon = inject<Widget>{"ButtonSet"};
  ColorButton m_maskColor;
  ContextBar* m_owner;
};

class ContextBar::PivotField : public ButtonSet {
public:
  PivotField() {
    setColumns(1);
    addItem(SkinTheme::instance()->parts.pivotCenter());

    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      base::Bind<void>(&PivotField::onPivotChange, this));

    onPivotChange();
  }

private:

  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    gfx::Rect bounds = this->bounds();

    Menu menu;
    CheckBox visible("Display pivot by default");
    HBox box;
    std::shared_ptr<ButtonSet> buttonset = inject<Widget>{"ButtonSet"};
    buttonset->setColumns(3);
    buttonset->addItem(theme->parts.pivotNorthwest());
    buttonset->addItem(theme->parts.pivotNorth());
    buttonset->addItem(theme->parts.pivotNortheast());
    buttonset->addItem(theme->parts.pivotWest());
    buttonset->addItem(theme->parts.pivotCenter());
    buttonset->addItem(theme->parts.pivotEast());
    buttonset->addItem(theme->parts.pivotSouthwest());
    buttonset->addItem(theme->parts.pivotSouth());
    buttonset->addItem(theme->parts.pivotSoutheast());
    box.addChild(buttonset);

    menu.addChild(&visible);
    menu.addChild(new MenuSeparator);
    menu.addChild(&box);

    bool isVisible = Preferences::instance().selection.pivotVisibility();
    app::gen::PivotPosition pos = Preferences::instance().selection.pivotPosition();
    visible.setSelected(isVisible);
    buttonset->setSelectedItem(int(pos));

    visible.Click.connect(
      [&visible](Event&){
        Preferences::instance().selection.pivotVisibility(
          visible.isSelected());
      });

    buttonset->ItemChange.connect(
      [&buttonset](ButtonSet::Item* item){
        Preferences::instance().selection.pivotPosition(
          app::gen::PivotPosition(buttonset->selectedItem()));
      });

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
  }

  void onPivotChange() {
    SkinTheme* theme = SkinTheme::instance();
    SkinPartPtr part;
    switch (Preferences::instance().selection.pivotPosition()) {
      case app::gen::PivotPosition::NORTHWEST: part = theme->parts.pivotNorthwest(); break;
      case app::gen::PivotPosition::NORTH:     part = theme->parts.pivotNorth(); break;
      case app::gen::PivotPosition::NORTHEAST: part = theme->parts.pivotNortheast(); break;
      case app::gen::PivotPosition::WEST:      part = theme->parts.pivotWest(); break;
      case app::gen::PivotPosition::CENTER:    part = theme->parts.pivotCenter(); break;
      case app::gen::PivotPosition::EAST:      part = theme->parts.pivotEast(); break;
      case app::gen::PivotPosition::SOUTHWEST: part = theme->parts.pivotSouthwest(); break;
      case app::gen::PivotPosition::SOUTH:     part = theme->parts.pivotSouth(); break;
      case app::gen::PivotPosition::SOUTHEAST: part = theme->parts.pivotSoutheast(); break;
    }
    if (part)
      getItem(0)->setIcon(part);
  }

};

class ContextBar::RotAlgorithmField : public ComboBox {
public:
  RotAlgorithmField() {
    // We use "m_lockChange" variable to avoid setting the rotation
    // algorithm when we call ComboBox::addItem() (because the first
    // addItem() generates an onChange() event).
    m_lockChange = true;
    addItem(new Item("Fast Rotation", tools::RotationAlgorithm::FAST));
    addItem(new Item("RotSprite", tools::RotationAlgorithm::ROTSPRITE));
    m_lockChange = false;

    setSelectedItemIndex((int)Preferences::instance().selection.rotationAlgorithm());
  }

protected:
  void onChange() override {
    if (m_lockChange)
      return;

    Preferences::instance().selection.rotationAlgorithm(
      static_cast<Item*>(getSelectedItem())->algo());
  }

  void onCloseListBox() override {
    releaseFocus();
  }

private:
  class Item : public ListItem {
  public:
    Item(const std::string& text, tools::RotationAlgorithm algo) :
      ListItem(text),
      m_algo(algo) {
    }

    tools::RotationAlgorithm algo() const { return m_algo; }

  private:
    tools::RotationAlgorithm m_algo;
  };

  bool m_lockChange;
};

class ContextBar::FreehandAlgorithmField : public CheckBox
{
public:
  FreehandAlgorithmField() : CheckBox("Pixel-perfect") {
    setup_mini_font(this);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    // Do nothing
  }

  void setFreehandAlgorithm(tools::FreehandAlgorithm algo) {
    switch (algo) {
      case tools::FreehandAlgorithm::DEFAULT:
        setSelected(false);
        break;
      case tools::FreehandAlgorithm::PIXEL_PERFECT:
        setSelected(true);
        break;
      case tools::FreehandAlgorithm::DOTS:
        // Not available
        break;
    }
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).freehandAlgorithm(
      isSelected() ?
        tools::FreehandAlgorithm::PIXEL_PERFECT:
        tools::FreehandAlgorithm::DEFAULT);

    releaseFocus();
  }
};

class ContextBar::SelectionModeField : public ButtonSet
{
public:
  SelectionModeField() {
    setColumns(3);
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

    addItem(theme->parts.selectionReplace());
    addItem(theme->parts.selectionAdd());
    addItem(theme->parts.selectionSubtract());

    setSelectedItem((int)Preferences::instance().selection.mode());
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), "Replace selection", BOTTOM);
    tooltipManager->addTooltipFor(at(1), "Add to selection\n(Shift)", BOTTOM);
    tooltipManager->addTooltipFor(at(2), "Subtract from selection\n(Shift+Alt)", BOTTOM);
  }

  void setSelectionMode(gen::SelectionMode mode) {
    setSelectedItem((int)mode, false);
    invalidate();
  }

protected:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    Preferences::instance().selection.mode(
      (gen::SelectionMode)selectedItem());
  }
};

class ContextBar::DropPixelsField : public ButtonSet
{
public:
  DropPixelsField() {
    setColumns(2);
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

    addItem(theme->parts.dropPixelsOk());
    addItem(theme->parts.dropPixelsCancel());
    setOfferCapture(false);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), "Drop pixels here", BOTTOM);
    tooltipManager->addTooltipFor(at(1), "Cancel drag and drop", BOTTOM);
  }

  base::Signal1<void, ContextBarObserver::DropAction> DropPixels;

protected:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    switch (selectedItem()) {
      case 0: DropPixels(ContextBarObserver::DropPixels); break;
      case 1: DropPixels(ContextBarObserver::CancelDrag); break;
    }
  }
};

class ContextBar::EyedropperField : public HBox
{
public:
  EyedropperField() {
    m_channel.addItem("Color+Alpha");
    m_channel.addItem("Color");
    m_channel.addItem("Alpha");
    m_channel.addItem("RGB+Alpha");
    m_channel.addItem("RGB");
    m_channel.addItem("HSB+Alpha");
    m_channel.addItem("HSB");
    m_channel.addItem("Gray+Alpha");
    m_channel.addItem("Gray");
    m_channel.addItem("Best fit Index");

    m_sample.addItem("All Layers");
    m_sample.addItem("Current Layer");

    addChild(new Label("Pick:"));
    addChild(&m_channel);
    addChild(new Label("Sample:"));
    addChild(&m_sample);

    m_channel.Change.connect(base::Bind<void>(&EyedropperField::onChannelChange, this));
    m_sample.Change.connect(base::Bind<void>(&EyedropperField::onSampleChange, this));
  }

  void updateFromPreferences(app::Preferences::Eyedropper& prefEyedropper) {
    m_channel.setSelectedItemIndex((int)prefEyedropper.channel());
    m_sample.setSelectedItemIndex((int)prefEyedropper.sample());
  }

private:
  void onChannelChange() {
    Preferences::instance().eyedropper.channel(
      (app::gen::EyedropperChannel)m_channel.getSelectedItemIndex());
  }

  void onSampleChange() {
    Preferences::instance().eyedropper.sample(
      (app::gen::EyedropperSample)m_sample.getSelectedItemIndex());
  }

  ComboBox m_channel;
  ComboBox m_sample;
};

class ContextBar::AutoSelectLayerField : public CheckBox
{
public:
  AutoSelectLayerField() : CheckBox("Auto Select Layer") {
    setup_mini_font(this);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Preferences::instance().editor.autoSelectLayer(isSelected());

    releaseFocus();
  }
};

class ContextBar::SymmetryField : public ButtonSet {
public:
  SymmetryField() {
    setColumns(2);
    setMultipleSelection(true);

    SkinTheme* theme = SkinTheme::instance();
    addItem(theme->parts.horizontalSymmetry());
    addItem(theme->parts.verticalSymmetry());
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), "Horizontal Symmetry", BOTTOM);
    tooltipManager->addTooltipFor(at(1), "Vertical Symmetry", BOTTOM);
  }

  void updateWithCurrentDocument() {
    Document* doc = UIContext::instance()->activeDocument();
    if (!doc)
      return;

    DocumentPreferences& docPref = Preferences::instance().document(doc);
    app::gen::SymmetryMode symmetryMode = docPref.symmetry.mode();

    at(0)->setSelected(symmetryMode == app::gen::SymmetryMode::HORIZONTAL
                        || symmetryMode == app::gen::SymmetryMode::BOTH);
    at(1)->setSelected(symmetryMode == app::gen::SymmetryMode::VERTICAL
                        || symmetryMode == app::gen::SymmetryMode::BOTH);
  }

private:
  void onItemChange(std::shared_ptr<Item> item) override {
    ButtonSet::onItemChange(item);

    Document* doc = UIContext::instance()->activeDocument();
    if (!doc)
      return;

    DocumentPreferences& docPref =
      Preferences::instance().document(doc);

    if (selectedItem() == -1)
      docPref.symmetry.mode(app::gen::SymmetryMode::NONE);
    else if (at(0)->isSelected() && at(1)->isSelected())
      docPref.symmetry.mode(app::gen::SymmetryMode::BOTH);
    else if (at(0)->isSelected())
      docPref.symmetry.mode(app::gen::SymmetryMode::HORIZONTAL);
    else
      docPref.symmetry.mode(app::gen::SymmetryMode::VERTICAL);

    // Redraw symmetry rules
    doc->notifyGeneralUpdate();
  }
};

ContextBar::ContextBar()
  : Box(HORIZONTAL)
{
  gfx::Border border = this->border();
  border.bottom(2*guiscale());
  setBorder(border);

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setBgColor(theme->colors.workspace());

  addChild(m_selectionOptionsBox = new HBox());
  m_selectionOptionsBox->addChild(m_dropPixels = new DropPixelsField());
  m_selectionOptionsBox->addChild(m_selectionMode = new SelectionModeField);
  m_selectionOptionsBox->addChild(m_transparentColor = new TransparentColorField(this));
  m_selectionOptionsBox->addChild(m_pivot = new PivotField);
  m_selectionOptionsBox->addChild(m_rotAlgo = new RotAlgorithmField());

  addChild(m_brushType = new BrushTypeField(this));
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));
  addChild(m_brushPatternField = new BrushPatternField());

  addChild(m_toleranceLabel = new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());
  addChild(m_contiguous = new ContiguousField());
  addChild(m_paintBucketSettings = new PaintBucketSettingsField());

  addChild(m_inkType = new InkTypeField(this));
  addChild(m_inkOpacityLabel = new Label("Opacity:"));
  addChild(m_inkOpacity = new InkOpacityField());
  addChild(m_inkShades = new InkShadesField());

  addChild(m_eyedropperField = new EyedropperField());

  addChild(m_autoSelectLayer = new AutoSelectLayerField());

  // addChild(new InkChannelTargetField());
  // addChild(new InkShadeField());
  // addChild(new InkSelectionField());

  addChild(m_sprayBox = new HBox());
  m_sprayBox->addChild(m_sprayLabel = new Label("Spray:"));
  m_sprayBox->addChild(m_sprayWidth = new SprayWidthField());
  m_sprayBox->addChild(m_spraySpeed = new SpraySpeedField());

  addChild(m_selectBoxHelp = new Label(""));

  setup_mini_font(m_sprayLabel);

  addChild(m_freehandBox = new HBox());
#if 0                           // TODO for v1.1
  m_freehandBox->addChild(m_freehandLabel = new Label("Freehand:"));
  setup_mini_font(m_freehandLabel);
#endif
  m_freehandBox->addChild(m_freehandAlgo = new FreehandAlgorithmField());

  setup_mini_font(m_toleranceLabel);
  setup_mini_font(m_inkOpacityLabel);

  addChild(m_symmetry = new SymmetryField());
  m_symmetry->setVisible(Preferences::instance().symmetryMode.enabled());

  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);

  tooltipManager->addTooltipFor(m_brushType, "Brush Type", BOTTOM);
  tooltipManager->addTooltipFor(m_brushSize, "Brush Size (in pixels)", BOTTOM);
  tooltipManager->addTooltipFor(m_brushAngle, "Brush Angle (in degrees)", BOTTOM);
  tooltipManager->addTooltipFor(m_inkType, "Ink", BOTTOM);
  tooltipManager->addTooltipFor(m_inkOpacity, "Opacity (paint intensity)", BOTTOM);
  tooltipManager->addTooltipFor(m_inkShades, "Shades", BOTTOM);
  tooltipManager->addTooltipFor(m_sprayWidth, "Spray Width", BOTTOM);
  tooltipManager->addTooltipFor(m_spraySpeed, "Spray Speed", BOTTOM);
  tooltipManager->addTooltipFor(m_pivot, "Rotation Pivot", BOTTOM);
  tooltipManager->addTooltipFor(m_transparentColor, "Transparent Color", BOTTOM);
  tooltipManager->addTooltipFor(m_rotAlgo, "Rotation Algorithm", BOTTOM);
  tooltipManager->addTooltipFor(m_freehandAlgo, "Freehand trace algorithm", BOTTOM);
  tooltipManager->addTooltipFor(m_paintBucketSettings, "Extra paint bucket options", BOTTOM);

  m_brushType->setupTooltips(tooltipManager);
  m_selectionMode->setupTooltips(tooltipManager);
  m_dropPixels->setupTooltips(tooltipManager);
  m_freehandAlgo->setupTooltips(tooltipManager);
  m_symmetry->setupTooltips(tooltipManager);

  App::instance()->activeToolManager()->addObserver(this);

  auto& pref = Preferences::instance();
  pref.symmetryMode.enabled.AfterChange.connect(
    base::Bind<void>(&ContextBar::onSymmetryModeChange, this));
  pref.colorBar.fgColor.AfterChange.connect(
    base::Bind<void>(&ContextBar::onFgOrBgColorChange, this, doc::Brush::ImageColor::MainColor));
  pref.colorBar.bgColor.AfterChange.connect(
    base::Bind<void>(&ContextBar::onFgOrBgColorChange, this, doc::Brush::ImageColor::BackgroundColor));

  m_dropPixels->DropPixels.connect(&ContextBar::onDropPixels, this);

  setActiveBrush(createBrushFromPreferences());
}

ContextBar::~ContextBar()
{
  App::instance()->activeToolManager()->removeObserver(this);
}

void ContextBar::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(0, 18*guiscale())); // TODO calculate height
}

void ContextBar::onToolSetOpacity(const int& newOpacity)
{
  if (g_updatingFromCode)
    return;

  m_inkOpacity->setTextf("%d", newOpacity);
}

void ContextBar::onToolSetFreehandAlgorithm()
{
  Tool* tool = App::instance()->activeTool();
  if (tool) {
    m_freehandAlgo->setFreehandAlgorithm(
      Preferences::instance().tool(tool).freehandAlgorithm());
  }
}

void ContextBar::onBrushSizeChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();

  updateForActiveTool();
}

void ContextBar::onBrushAngleChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();
}

void ContextBar::onActiveToolChange(tools::Tool* tool)
{
  if (m_activeBrush->type() != kImageBrushType)
    setActiveBrush(ContextBar::createBrushFromPreferences());
  else {
    updateForTool(tool);
  }
}

void ContextBar::onSymmetryModeChange()
{
  updateForActiveTool();
}

void ContextBar::onFgOrBgColorChange(doc::Brush::ImageColor imageColor)
{
  if (!m_activeBrush)
    return;

  if (m_activeBrush->type() == kImageBrushType) {
    ASSERT(m_activeBrush->image());

    auto& pref = Preferences::instance();
    m_activeBrush->setImageColor(
      imageColor,
      color_utils::color_for_image(
        (imageColor == doc::Brush::ImageColor::MainColor ?
         pref.colorBar.fgColor():
         pref.colorBar.bgColor()),
        m_activeBrush->image()->pixelFormat()));
  }
}

void ContextBar::onDropPixels(ContextBarObserver::DropAction action)
{
  notifyObservers(&ContextBarObserver::onDropPixels, action);
}

void ContextBar::updateForActiveTool()
{
  updateForTool(App::instance()->activeTool());
}

void ContextBar::updateForTool(tools::Tool* tool)
{
  // TODO Improve the design of the visibility of ContextBar
  // items. Actually this manual show/hide logic is a mess. There
  // should be a IContextBarUser interface, with a method to ask who
  // needs which items to be visible. E.g. different tools elements
  // (inks, controllers, etc.) and sprite editor states are the main
  // target to implement this new IContextBarUser and ask for
  // ContextBar elements.

  base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

  ToolPreferences* toolPref = nullptr;
  ToolPreferences::Brush* brushPref = nullptr;
  Preferences& preferences = Preferences::instance();

  if (tool) {
    toolPref = &preferences.tool(tool);
    brushPref = &toolPref->brush;
  }

  if (toolPref) {
    m_sizeConn = brushPref->size.AfterChange.connect(base::Bind<void>(&ContextBar::onBrushSizeChange, this));
    m_angleConn = brushPref->angle.AfterChange.connect(base::Bind<void>(&ContextBar::onBrushAngleChange, this));
    m_opacityConn = toolPref->opacity.AfterChange.connect(&ContextBar::onToolSetOpacity, this);
    m_freehandAlgoConn = toolPref->freehandAlgorithm.AfterChange.connect(base::Bind<void>(&ContextBar::onToolSetFreehandAlgorithm, this));
  }

  if (tool)
    m_brushType->updateBrush(tool);

  if (brushPref) {
    m_brushSize->setTextf("%d", brushPref->size());
    m_brushAngle->setTextf("%d", brushPref->angle());
  }

  m_brushPatternField->setBrushPattern(
    preferences.brush.pattern());

  // Tool ink
  bool isPaint = tool &&
    (tool->getInk(0)->isPaint() ||
     tool->getInk(1)->isPaint());
  bool isEffect = tool &&
    (tool->getInk(0)->isEffect() ||
     tool->getInk(1)->isEffect());

  // True if the current tool support opacity slider
  bool supportOpacity = (isPaint || isEffect);

  // True if it makes sense to change the ink property for the current
  // tool.
  bool hasInk = tool &&
    ((tool->getInk(0)->isPaint() && !tool->getInk(0)->isEffect()) ||
     (tool->getInk(1)->isPaint() && !tool->getInk(1)->isEffect()));

  bool hasInkWithOpacity = false;
  bool hasInkShades = false;

  if (toolPref) {
    m_tolerance->setTextf("%d", toolPref->tolerance());
    m_contiguous->setSelected(toolPref->contiguous());

    m_inkType->setInkTypeIcon(toolPref->ink());
    m_inkOpacity->setTextf("%d", toolPref->opacity());

    hasInkWithOpacity =
      ((isPaint && tools::inkHasOpacity(toolPref->ink())) ||
       (isEffect));

    hasInkShades =
      (isPaint && !isEffect && toolPref->ink() == InkType::SHADING);

    m_freehandAlgo->setFreehandAlgorithm(toolPref->freehandAlgorithm());

    m_sprayWidth->setValue(toolPref->spray.width());
    m_spraySpeed->setValue(toolPref->spray.speed());
  }

  bool updateShade = (!m_inkShades->isVisible() && hasInkShades);

  m_eyedropperField->updateFromPreferences(preferences.eyedropper);
  m_autoSelectLayer->setSelected(preferences.editor.autoSelectLayer());

  // True if we have an image as brush
  bool hasImageBrush = (activeBrush()->type() == kImageBrushType);

  // True if the brush type supports angle.
  bool hasBrushWithAngle =
    (activeBrush()->size() > 1) &&
    (activeBrush()->type() == kSquareBrushType ||
     activeBrush()->type() == kLineBrushType);

  // True if the current tool is eyedropper.
  bool isEyedropper = tool &&
    (tool->getInk(0)->isEyedropper() ||
     tool->getInk(1)->isEyedropper());

  // True if the current tool is move tool.
  bool isMove = tool &&
    (tool->getInk(0)->isCelMovement() ||
     tool->getInk(1)->isCelMovement());

  // True if the current tool is floodfill
  bool isFloodfill = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs tolerance options
  bool hasTolerance = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs spray options
  bool hasSprayOptions = tool &&
    (tool->getPointShape(0)->isSpray() ||
     tool->getPointShape(1)->isSpray());

  bool hasSelectOptions = tool &&
    (tool->getInk(0)->isSelection() ||
     tool->getInk(1)->isSelection());

  bool isFreehand = tool &&
    (tool->getController(0)->isFreehand() ||
     tool->getController(1)->isFreehand());

  bool showOpacity =
    (supportOpacity) &&
    ((isPaint && (hasInkWithOpacity || hasImageBrush)) ||
     (isEffect));

  // Show/Hide fields
  m_brushType->setVisible(supportOpacity && (!isFloodfill || (isFloodfill && hasImageBrush)));
  m_brushSize->setVisible(supportOpacity && !isFloodfill && !hasImageBrush);
  m_brushAngle->setVisible(supportOpacity && !isFloodfill && !hasImageBrush && hasBrushWithAngle);
  m_brushPatternField->setVisible(supportOpacity && hasImageBrush);
  m_inkType->setVisible(hasInk);
  m_inkOpacityLabel->setVisible(showOpacity);
  m_inkOpacity->setVisible(showOpacity);
  m_inkShades->setVisible(hasInkShades);
  m_eyedropperField->setVisible(isEyedropper);
  m_autoSelectLayer->setVisible(isMove);
  m_freehandBox->setVisible(isFreehand && supportOpacity);
  m_toleranceLabel->setVisible(hasTolerance);
  m_tolerance->setVisible(hasTolerance);
  m_contiguous->setVisible(hasTolerance);
  m_paintBucketSettings->setVisible(hasTolerance);
  m_sprayBox->setVisible(hasSprayOptions);
  m_selectionOptionsBox->setVisible(hasSelectOptions);
  m_selectionMode->setVisible(true);
  m_pivot->setVisible(true);
  m_dropPixels->setVisible(false);
  m_selectBoxHelp->setVisible(false);

  m_symmetry->setVisible(
    Preferences::instance().symmetryMode.enabled() &&
    (isPaint || isEffect || hasSelectOptions));
  m_symmetry->updateWithCurrentDocument();

  // Update ink shades with the current selected palette entries
  if (updateShade)
    m_inkShades->updateShadeFromColorBarPicks();

  layout();
}

void ContextBar::updateForMovingPixels()
{
  tools::Tool* tool = App::instance()->toolBox()->getToolById(
    tools::WellKnownTools::RectangularMarquee);
  if (tool)
    updateForTool(tool);

  m_dropPixels->deselectItems();
  m_dropPixels->setVisible(true);
  m_selectionMode->setVisible(false);
  layout();
}

void ContextBar::updateForSelectingBox(const std::string& text)
{
  if (m_selectBoxHelp->isVisible() && m_selectBoxHelp->text() == text)
    return;

  updateForTool(nullptr);
  m_selectBoxHelp->setText(text);
  m_selectBoxHelp->setVisible(true);
  layout();
}

void ContextBar::updateToolLoopModifiersIndicators(tools::ToolLoopModifiers modifiers)
{
  if (!m_selectionMode->isVisible())
    return;

  gen::SelectionMode mode = gen::SelectionMode::DEFAULT;
  if (int(modifiers) & int(tools::ToolLoopModifiers::kAddSelection))
    mode = gen::SelectionMode::ADD;
  else if (int(modifiers) & int(tools::ToolLoopModifiers::kSubtractSelection))
    mode = gen::SelectionMode::SUBTRACT;

  m_selectionMode->setSelectionMode(mode);
}

void ContextBar::updateAutoSelectLayer(bool state)
{
  if (!m_autoSelectLayer->isVisible())
    return;

  m_autoSelectLayer->setSelected(state);
}

void ContextBar::setActiveBrushBySlot(int slot)
{
  AppBrushes& brushes = App::instance()->brushes();
  BrushSlot brush = brushes.getBrushSlot(slot);
  if (!brush.isEmpty()) {
    brushes.lockBrushSlot(slot);

    Tool* tool = App::instance()->activeTool();
    Preferences& pref = Preferences::instance();
    ToolPreferences& toolPref = pref.tool(tool);
    ToolPreferences::Brush& brushPref = toolPref.brush;

    if (brush.brush()) {
      if (brush.brush()->type() == doc::kImageBrushType) {
        setActiveBrush(brush.brush());
      }
      else {
        if (brush.hasFlag(BrushSlot::Flags::BrushType))
          brushPref.type(static_cast<app::gen::BrushType>(brush.brush()->type()));

        if (brush.hasFlag(BrushSlot::Flags::BrushSize))
          brushPref.size(brush.brush()->size());

        if (brush.hasFlag(BrushSlot::Flags::BrushAngle))
          brushPref.angle(brush.brush()->angle());

        setActiveBrush(ContextBar::createBrushFromPreferences());
      }
    }

    if (brush.hasFlag(BrushSlot::Flags::FgColor))
      pref.colorBar.fgColor(brush.fgColor());

    if (brush.hasFlag(BrushSlot::Flags::BgColor))
      pref.colorBar.bgColor(brush.bgColor());

    if (brush.hasFlag(BrushSlot::Flags::InkType))
      setInkType(brush.inkType());

    if (brush.hasFlag(BrushSlot::Flags::InkOpacity))
      toolPref.opacity(brush.inkOpacity());

    if (brush.hasFlag(BrushSlot::Flags::Shade))
      m_inkShades->setShade(brush.shade());

    if (brush.hasFlag(BrushSlot::Flags::PixelPerfect))
      toolPref.freehandAlgorithm(
        (brush.pixelPerfect() ?
         tools::FreehandAlgorithm::PIXEL_PERFECT:
         tools::FreehandAlgorithm::REGULAR));
  }
  else {
    updateForTool(App::instance()->activeTool());
    m_brushType->showPopupAndHighlightSlot(slot);
  }
}

void ContextBar::setActiveBrush(const doc::BrushRef& brush)
{
  if (brush->type() == kImageBrushType)
    m_activeBrush = brush;
  else {
    Tool* tool = App::instance()->activeTool();
    auto& brushPref = Preferences::instance().tool(tool).brush;
    auto newBrushType = static_cast<app::gen::BrushType>(brush->type());
    if (brushPref.type() != newBrushType)
      brushPref.type(newBrushType);

    m_activeBrush = brush;
  }

  BrushChange();

  updateForActiveTool();
}

doc::BrushRef ContextBar::activeBrush(tools::Tool* tool) const
{
  if ((!tool) ||
      (tool == App::instance()->activeTool()) ||
      (tool->getInk(0)->isPaint() &&
       m_activeBrush->type() == kImageBrushType)) {
    m_activeBrush->setPattern(Preferences::instance().brush.pattern());
    return m_activeBrush;
  }

  return ContextBar::createBrushFromPreferences(
    &Preferences::instance().tool(tool).brush);
}

void ContextBar::discardActiveBrush()
{
  setActiveBrush(ContextBar::createBrushFromPreferences());
}

// static
doc::BrushRef ContextBar::createBrushFromPreferences(ToolPreferences::Brush* brushPref)
{
  if (brushPref == nullptr) {
    tools::Tool* tool = App::instance()->activeTool();
    brushPref = &Preferences::instance().tool(tool).brush;
  }

  doc::BrushRef brush;
  brush.reset(
    new Brush(
      static_cast<doc::BrushType>(brushPref->type()),
      brushPref->size(),
      brushPref->angle()));
  return brush;
}

BrushSlot ContextBar::createBrushSlotFromPreferences()
{
  tools::Tool* activeTool = App::instance()->activeTool();
  auto& pref = Preferences::instance();
  auto& saveBrush = pref.saveBrush;
  auto& toolPref = pref.tool(activeTool);

  int flags = 0;
  if (saveBrush.brushType()) flags |= int(BrushSlot::Flags::BrushType);
  if (saveBrush.brushSize()) flags |= int(BrushSlot::Flags::BrushSize);
  if (saveBrush.brushAngle()) flags |= int(BrushSlot::Flags::BrushAngle);
  if (saveBrush.fgColor()) flags |= int(BrushSlot::Flags::FgColor);
  if (saveBrush.bgColor()) flags |= int(BrushSlot::Flags::BgColor);
  if (saveBrush.inkType()) flags |= int(BrushSlot::Flags::InkType);
  if (saveBrush.inkOpacity()) flags |= int(BrushSlot::Flags::InkOpacity);
  if (saveBrush.shade()) flags |= int(BrushSlot::Flags::Shade);
  if (saveBrush.pixelPerfect()) flags |= int(BrushSlot::Flags::PixelPerfect);

  return BrushSlot(
    BrushSlot::Flags(flags),
    activeBrush(activeTool),
    pref.colorBar.fgColor(),
    pref.colorBar.bgColor(),
    toolPref.ink(),
    toolPref.opacity(),
    getShade(),
    toolPref.freehandAlgorithm() == tools::FreehandAlgorithm::PIXEL_PERFECT);
}

Shade ContextBar::getShade() const
{
  return m_inkShades->getShade();
}

doc::Remap* ContextBar::createShadeRemap(bool left)
{
  return m_inkShades->createShadeRemap(left);
}

void ContextBar::reverseShadeColors()
{
  m_inkShades->reverseShadeColors();
}

void ContextBar::setInkType(tools::InkType type)
{
  m_inkType->setInkType(type);
}

} // namespace app
