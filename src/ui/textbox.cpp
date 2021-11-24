// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/textbox.h"

#include "gfx/size.h"
#include "ui/events/size_hint_event.h"
#include "ui/intern.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

namespace ui {

TextBox::TextBox(const std::string& text, int align)
 : Widget(kTextBoxWidget)
{
  setFocusStop(true);
  setAlign(align);
  setText(text);
  initTheme();
}

bool TextBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage:
      if (hasFocus()) {
        View* view = View::getView(this);
        if (view) {
          gfx::Rect vp = view->viewportBounds();
          gfx::Point scroll = view->viewScroll();
          int textheight = textHeight();

          switch (static_cast<KeyMessage*>(msg)->scancode()) {

            case kKeyLeft:
              scroll.x -= vp.w/2;
              view->setViewScroll(scroll);
              break;

            case kKeyRight:
              scroll.x += vp.w/2;
              view->setViewScroll(scroll);
              break;

            case kKeyUp:
              scroll.y -= vp.h/2;
              view->setViewScroll(scroll);
              break;

            case kKeyDown:
              scroll.y += vp.h/2;
              view->setViewScroll(scroll);
              break;

            case kKeyPageUp:
              scroll.y -= (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case kKeyPageDown:
              scroll.y += (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case kKeyHome:
              scroll.y = 0;
              view->setViewScroll(scroll);
              break;

            case kKeyEnd:
              scroll.y = bounds().h - vp.h;
              view->setViewScroll(scroll);
              break;

            default:
              return Widget::onProcessMessage(msg);
          }
        }
        return true;
      }
      break;

    case kMouseDownMessage: {
      View* view = View::getView(this);
      if (view) {
        captureMouse();
        m_oldPos = static_cast<MouseMessage*>(msg)->position();
        set_mouse_cursor(kScrollCursor);
        return true;
      }
      break;
    }

    case kMouseMoveMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        gfx::Point scroll = view->viewScroll();
        gfx::Point newPos = static_cast<MouseMessage*>(msg)->position();

        scroll += m_oldPos - newPos;
        view->setViewScroll(scroll);

        m_oldPos = newPos;
      }
      break;
    }

    case kMouseUpMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        releaseMouse();
        set_mouse_cursor(kArrowCursor);
        return true;
      }
      break;
    }

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->viewScroll();

        scroll += static_cast<MouseMessage*>(msg)->wheelDelta() * textHeight()*3;

        view->setViewScroll(scroll);
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

void TextBox::onPaint(PaintEvent& ev)
{
  theme()->paintTextBox(ev);
}

void TextBox::onSizeHint(SizeHintEvent& ev)
{
  int w = 0;
  int h = 0;

  // TODO is it necessary?
  //w = widget->border_width.l + widget->border_width.r;
  //h = widget->border_width.t + widget->border_width.b;

  drawTextBox(NULL, this, &w, &h, gfx::ColorNone, gfx::ColorNone);

  if (this->align() & WORDWRAP) {
    View* view = View::getView(this);
    int width, min = w;

    if (view) {
      width = view->viewportBounds().w;
    }
    else {
      width = bounds().w;
    }

    w = MAX(min, width);
    drawTextBox(NULL, this, &w, &h, gfx::ColorNone, gfx::ColorNone);

    w = min;
  }

  ev.setSizeHint(gfx::Size(w, h));
}

void TextBox::onSetText()
{
  View* view = View::getView(this);
  if (view)
    view->updateView();

  Widget::onSetText();
}

} // namespace ui
