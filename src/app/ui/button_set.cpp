// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/button_set.h"

#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "gfx/color.h"
#include "she/surface.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cstdarg>

namespace app {

using namespace ui;
using namespace app::skin;

// Last selected item for ButtonSet activated on mouse up when the
// mouse capture is get.
static int g_itemBeforeCapture = -1;

WidgetType buttonset_item_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

static Widget::Shared<ButtonSet::Item> _bsireg{"ButtonSetItem"};

ButtonSet::Item::Item()
  : Widget(buttonset_item_type())
  , m_icon(NULL)
{
  setup_mini_font(this);
  setAlign(CENTER | MIDDLE);
  setFocusStop(true);
}

void ButtonSet::Item::setIcon(const SkinPartPtr& icon, bool mono)
{
  m_icon = icon;
  m_mono = mono;
  invalidate();
}

ButtonSet* ButtonSet::Item::buttonSet()
{
  return static_cast<ButtonSet*>(parent());
}

void ButtonSet::Item::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  Graphics* g = ev.graphics();
  gfx::Rect rc = clientBounds();
  gfx::Color fg, bg;
  SkinPartPtr nw;
  gfx::Rect boxRc, textRc, iconRc;
  gfx::Size iconSize;
  if (m_icon)
    iconSize = m_icon->size();

  getTextIconInfo(
    &boxRc, &textRc, &iconRc,
    CENTER | (hasText() ? BOTTOM: MIDDLE),
    iconSize.w, iconSize.h);

  Grid::Info info = buttonSet()->getChildInfo(this);
  bool isLastCol = (info.col+info.hspan >= info.grid_cols);
  bool isLastRow = (info.row+info.vspan >= info.grid_rows);

  if (m_icon || isLastRow) {
    textRc.y -= 1*guiscale();
    iconRc.y -= 1*guiscale();
  }

  if (!gfx::is_transparent(bgColor()))
    g->fillRect(bgColor(), g->getClipBounds());

  if (isSelected() || hasMouseOver()) {
    if (hasCapture()) {
      nw = theme->parts.toolbuttonPushed();
      fg = theme->colors.buttonSelectedText();
      bg = theme->colors.buttonSelectedFace();
    }
    else {
      nw = (hasFocus() ? theme->parts.toolbuttonHotFocused():
                         theme->parts.toolbuttonHot());
      fg = theme->colors.buttonHotText();
      bg = theme->colors.buttonHotFace();
    }
  }
  else {
    nw = (hasFocus() ? theme->parts.toolbuttonFocused():
                       theme->parts.toolbuttonLast());
    fg = theme->colors.buttonNormalText();
    bg = theme->colors.buttonNormalFace();
  }

  if (!isLastCol)
    rc.w += 1*guiscale();

  if (!isLastRow) {
    if (nw == theme->parts.toolbuttonHotFocused())
      rc.h += 2*guiscale();
    else
      rc.h += 3*guiscale();
  }

  theme->drawRect(g, rc, nw.get(), bg);

  if (m_icon) {
    she::Surface* bmp = m_icon->bitmap(0);

    if (isSelected() && hasCapture())
      g->drawColoredRgbaSurface(bmp, theme->colors.buttonSelectedText(),
                                iconRc.x, iconRc.y);
    else if (m_mono)
      g->drawColoredRgbaSurface(bmp, theme->colors.buttonNormalText(),
                                iconRc.x, iconRc.y);
    else
      g->drawRgbaSurface(bmp, iconRc.x, iconRc.y);
  }

  if (hasText()) {
    g->setFont(font());
    g->drawUIString(text(), fg, gfx::ColorNone, textRc.origin(),
                    false);
  }
}

bool ButtonSet::Item::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      if (isEnabled()) {
        // TODO theme specific stuff
        invalidate();
      }
      break;

    case ui::kKeyDownMessage:
      if (isEnabled() && hasText()) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        bool mnemonicPressed =
          (msg->altPressed() &&
           mnemonicChar() &&
           mnemonicChar() == tolower(keymsg->unicodeChar()));

        if (mnemonicPressed || (hasFocus() && keymsg->scancode() == kKeySpace)) {
          buttonSet()->setSelectedItem(std::static_pointer_cast<Item>(shared_from_this()));
          onClick();
        }
      }
      break;

    case ui::kMouseDownMessage:
      // Only for single-item and trigerred on mouse up ButtonSets: We
      // save the current selected item to restore it just in case the
      // user leaves the ButtonSet without releasing the mouse button
      // and the mouse capture if offered to other ButtonSet.
      if (buttonSet()->m_triggerOnMouseUp) {
        ASSERT(g_itemBeforeCapture < 0);
        g_itemBeforeCapture = buttonSet()->selectedItem();
      }

      captureMouse();
      buttonSet()->setSelectedItem(std::static_pointer_cast<Item>(shared_from_this()));
      invalidate();

      if (static_cast<MouseMessage*>(msg)->left() &&
          !buttonSet()->m_triggerOnMouseUp) {
        onClick();
      }
      break;

    case ui::kMouseUpMessage:
      if (hasCapture()) {
        if (g_itemBeforeCapture >= 0)
          g_itemBeforeCapture = -1;

        releaseMouse();
        invalidate();

        if (static_cast<MouseMessage*>(msg)->left()) {
          if (buttonSet()->m_triggerOnMouseUp)
            onClick();
        }
        else if (static_cast<MouseMessage*>(msg)->right()) {
          onRightClick();
        }
      }
      break;

    case ui::kMouseMoveMessage:
      if (hasCapture()) {
        if (buttonSet()->m_offerCapture) {
          if (offerCapture(static_cast<ui::MouseMessage*>(msg), buttonset_item_type())) {
            // Only for ButtonSets trigerred on mouse up.
            if (buttonSet()->m_triggerOnMouseUp &&
                g_itemBeforeCapture >= 0) {
              // As we never received a kMouseUpMessage (so we never
              // called onClick()), we have to restore the selected
              // item at the point when we received the mouse capture.
              buttonSet()->setSelectedItem(g_itemBeforeCapture);
              g_itemBeforeCapture = -1;
            }
          }
        }
      }
      break;

    case ui::kMouseLeaveMessage:
    case ui::kMouseEnterMessage:
      invalidate();
      break;
  }
  return Widget::onProcessMessage(msg);
}

void ButtonSet::Item::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size iconSize;
  if (m_icon) {
    iconSize = m_icon->size();
    iconSize.w = MAX(iconSize.w, 16*guiscale());
    iconSize.h = MAX(iconSize.h, 16*guiscale());
  }

  gfx::Rect boxRc;
  getTextIconInfo(
    &boxRc, NULL, NULL,
    CENTER | (hasText() ? BOTTOM: MIDDLE),
    iconSize.w, iconSize.h);

  gfx::Size sz = boxRc.size();
  if (hasText())
    sz += 8*guiscale();

  Grid::Info info = buttonSet()->getChildInfo(this);
  if (info.row == info.grid_rows-1)
    sz.h += 3*guiscale();

  ev.setSizeHint(sz);
}

void ButtonSet::Item::onClick()
{
  buttonSet()->onItemChange(std::static_pointer_cast<ButtonSet::Item>(shared_from_this()));
}

ButtonSet::ButtonSet() : Grid(1, false) {
  noBorderNoChildSpacing();
}

std::shared_ptr<ButtonSet::Item> ButtonSet::addItem(const std::string& text, int hspan, int vspan)
{
  std::shared_ptr<Item> item = inject<ui::Widget>{"ButtonSetItem"};
  item->setText(text);
  return addItem(item, hspan, vspan);
}

std::shared_ptr<ButtonSet::Item> ButtonSet::addItem(const skin::SkinPartPtr& icon, int hspan, int vspan)
{
  std::shared_ptr<Item> item = inject<ui::Widget>{"ButtonSetItem"};
  item->setIcon(icon);
  return addItem(item, hspan, vspan);
}

std::shared_ptr<ButtonSet::Item> ButtonSet::addItem(std::shared_ptr<Item> item, int hspan, int vspan)
{
  addChildInCell(item, hspan, vspan, HORIZONTAL | VERTICAL);
  return item;
}

std::shared_ptr<ButtonSet::Item> ButtonSet::getItem(int index)
{
  return std::dynamic_pointer_cast<Item>(at(index)->shared_from_this());
}

int ButtonSet::selectedItem() const
{
  int index = 0;
  for (Widget* child : children()) {
    if (child->isSelected())
      return index;
    ++index;
  }
  return -1;
}

void ButtonSet::setSelectedItem(int index, bool focusItem)
{
  if (index >= 0 && index < (int)children().size())
    setSelectedItem(std::static_pointer_cast<Item>(at(index)->shared_from_this()), focusItem);
  else
    setSelectedItem(nullptr, focusItem);
}

void ButtonSet::setSelectedItem(std::shared_ptr<Item> item, bool focusItem)
{
  if (!m_multipleSelection) {
    if (item && item->isSelected())
      return;

    auto sel = findSelectedItem();
    if (sel)
      sel->setSelected(false);
  }

  if (item) {
    item->setSelected(!item->isSelected());
    if (focusItem)
      item->requestFocus();
  }
}

void ButtonSet::deselectItems()
{
  auto sel = findSelectedItem();
  if (sel)
    sel->setSelected(false);
}

void ButtonSet::setOfferCapture(bool state)
{
  m_offerCapture = state;
}

void ButtonSet::setTriggerOnMouseUp(bool state)
{
  m_triggerOnMouseUp = state;
}

void ButtonSet::setMultipleSelection(bool state)
{
  m_multipleSelection = state;
}

void ButtonSet::onItemChange(std::shared_ptr<Item> item)
{
  ItemChange(item.get());
}

void ButtonSet::onRightClick(std::shared_ptr<Item> item)
{
  RightClick(item.get());
}

std::shared_ptr<ButtonSet::Item> ButtonSet::findSelectedItem() const
{
  for (auto child : children()) {
    if (child->isSelected())
      return std::static_pointer_cast<Item>(child->shared_from_this());
  }
  return nullptr;
}

static ui::Widget::Shared<ButtonSet> _bs{"ButtonSet"};

} // namespace app
