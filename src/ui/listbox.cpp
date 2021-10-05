// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/listbox.h"
#include "base/iterator.h"
#include "base/path.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

namespace ui {

using namespace gfx;

ListBox::ListBox()
  : Widget(kListBoxWidget)
{
  setFocusStop(true);
  initTheme();
}

Widget* ListBox::getSelectedChild()
{
  for (auto child : children())
    if (child->isSelected())
      return child;

  return nullptr;
}

int ListBox::getSelectedIndex()
{
  int i = 0;

  for (auto child : children()) {
    if (child->isSelected())
      return i;

    i++;
  }

  return -1;
}

void ListBox::selectChild(Widget* item)
{
  for (auto child : children()) {
    if (child->isSelected()) {
      if (item && child == item)
        return;

      child->setSelected(false);
    }
  }

  if (item) {
    item->setSelected(true);
    makeChildVisible(item);
  }

  onChange();
}

void ListBox::selectIndex(int index)
{
  const WidgetsList& children = this->children();
  if (index < 0 || index >= (int)children.size())
    return;

  ListItem* child = static_cast<ListItem*>(children[index]);
  ASSERT(child);
  selectChild(child);
}

std::size_t ListBox::getItemsCount() const
{
  return children().size();
}

void ListBox::makeChildVisible(Widget* child)
{
  View* view = View::getView(this);
  if (!view)
    return;

  gfx::Point scroll = view->viewScroll();
  gfx::Rect vp = view->viewportBounds();

  if (child->bounds().y < vp.y)
    scroll.y = child->bounds().y - bounds().y;
  else if (child->bounds().y > vp.y + vp.h - child->bounds().h)
    scroll.y = (child->bounds().y - bounds().y
                - vp.h + child->bounds().h);

  view->setViewScroll(scroll);
}

// Setup the scroll to center the selected item in the viewport
void ListBox::centerScroll()
{
  View* view = View::getView(this);
  Widget* item = getSelectedChild();

  if (view && item) {
    gfx::Rect vp = view->viewportBounds();
    gfx::Point scroll = view->viewScroll();

    scroll.y = ((item->bounds().y - bounds().y)
                - vp.h/2 + item->bounds().h/2);

    view->setViewScroll(scroll);
  }
}

inline bool sort_by_text(Widget* a, Widget* b) {
  return (base::compare_filenames(a->text(), b->text()) < 0);
}

void ListBox::sortItems()
{
  WidgetsList widgets = children();
  std::sort(widgets.begin(), widgets.end(), &sort_by_text);

  // Remove all children and add then again.
  removeAllChildren();
  for (Widget* child : widgets)
    addChild(child);
}

bool ListBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      centerScroll();
      break;

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        int select = getSelectedIndex();
        View* view = View::getView(this);
        bool pick_item = true;

        if (view) {
          gfx::Rect vp = view->viewportBounds();

          if (mousePos.y < vp.y) {
            int num = MAX(1, (vp.y - mousePos.y) / 8);
            selectIndex(select-num);
            pick_item = false;
          }
          else if (mousePos.y >= vp.y + vp.h) {
            int num = MAX(1, (mousePos.y - (vp.y+vp.h-1)) / 8);
            selectIndex(select+num);
            pick_item = false;
          }
        }

        if (pick_item) {
          Widget* picked;

          if (view) {
            picked = view->viewport()->pick(mousePos);
          }
          else {
            picked = pick(mousePos);
          }

          /* if the picked widget is a child of the list, select it */
          if (picked && hasChild(picked)) {
            if (ListItem* pickedItem = dynamic_cast<ListItem*>(picked))
              selectChild(pickedItem);
          }
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      releaseMouse();
      break;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->viewScroll();
        scroll += static_cast<MouseMessage*>(msg)->wheelDelta() * textHeight()*3;
        view->setViewScroll(scroll);
      }
      break;
    }

    case kKeyDownMessage:
      if (hasFocus() && !children().empty()) {
        int select = getSelectedIndex();
        View* view = View::getView(this);
        int bottom = MAX(0, children().size()-1);
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);

        switch (keymsg->scancode()) {
          case kKeyUp:
            // Select previous element.
            if (select >= 0)
              select--;
            // Or select the bottom of the list if there is no
            // selected item.
            else
              select = bottom;
            break;
          case kKeyDown:
            select++;
            break;
          case kKeyHome:
            select = 0;
            break;
          case kKeyEnd:
            select = bottom;
            break;
          case kKeyPageUp:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              select -= vp.h / textHeight();
            }
            else
              select = 0;
            break;
          case kKeyPageDown:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              select += vp.h / textHeight();
            }
            else
              select = bottom;
            break;
          case kKeyLeft:
          case kKeyRight:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              gfx::Point scroll = view->viewScroll();
              int sgn = (keymsg->scancode() == kKeyLeft) ? -1: 1;

              scroll.x += vp.w/2*sgn;

              view->setViewScroll(scroll);
            }
            break;
          default:
            return Widget::onProcessMessage(msg);
        }

        selectIndex(MID(0, select, bottom));
        return true;
      }
      break;

    case kDoubleClickMessage:
      onDoubleClickItem();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ListBox::onPaint(PaintEvent& ev)
{
  theme()->paintListBox(ev);
}

void ListBox::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  Rect cpos = childrenBounds();

  for (auto child : children()) {
    cpos.h = child->sizeHint().h;
    child->setBounds(cpos);

    cpos.y += child->bounds().h + this->childSpacing();
  }
}

void ListBox::onSizeHint(SizeHintEvent& ev)
{
  int w = 0, h = 0;
  for (std::size_t i = 0, end = children().size(); i < end; ++i) {
    Size reqSize = static_cast<ListItem*>(at(i))->sizeHint();

    w = MAX(w, reqSize.w);
    h += reqSize.h + (i+1 != end ? this->childSpacing(): 0);
  }

  w += border().width();
  h += border().height();

  ev.setSizeHint(Size(w, h));
}

void ListBox::onChange()
{
  Change();
}

void ListBox::onDoubleClickItem()
{
  DoubleClickItem();
}

} // namespace ui
