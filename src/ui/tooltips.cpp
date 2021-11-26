// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/tooltips.h"

#include "base/unique_ptr.h"
#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <string>

static const int kTooltipDelayMsecs = 300;

namespace ui {

using namespace gfx;

TooltipManager::TooltipManager()
  : Widget(kGenericWidget)
{
  Manager* manager = Manager::getDefault();
  manager->addMessageFilter(kMouseEnterMessage, this);
  manager->addMessageFilter(kKeyDownMessage, this);
  manager->addMessageFilter(kMouseDownMessage, this);
  manager->addMessageFilter(kMouseLeaveMessage, this);

  setVisible(false);
}

void TooltipManager::addTooltipFor(std::shared_ptr<Widget> widget, const std::string& text, int arrowAlign) {
  m_tips[widget.get()] = TipInfo{text, arrowAlign};
}

void TooltipManager::addTooltipFor(Widget* widget, const std::string& text, int arrowAlign) {
  m_tips[widget] = TipInfo{text, arrowAlign};
}

void TooltipManager::removeTooltipFor(Widget* widget) {
  m_tips.erase(widget);
}

bool TooltipManager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage: {
      for (Widget* widget : msg->recipients()) {
        Tips::iterator it = m_tips.find(widget);
        if (it != m_tips.end()) {
          m_target.widget = it->first;
          m_target.tipInfo = it->second;

          if (!m_timer) {
            m_timer = Timer::create(kTooltipDelayMsecs, *this);
            m_timer->Tick.connect(&TooltipManager::onTick, this);
          }

          m_timer->start();
        }
      }
      return false;
    }

    case kKeyDownMessage:
    case kMouseDownMessage:
    case kMouseLeaveMessage:
      if (m_tipWindow) {
        m_tipWindow->closeWindow(NULL);
        m_tipWindow.reset();
      }

      if (m_timer)
        m_timer->stop();

      return false;
  }

  return Widget::onProcessMessage(msg);
}

void TooltipManager::onTick()
{
  if (!m_tipWindow) {
    m_tipWindow.reset(new TipWindow(m_target.tipInfo.text));

    int arrowAlign = m_target.tipInfo.arrowAlign;
    gfx::Rect target = m_target.widget->bounds();
    if (!arrowAlign)
      target.setOrigin(ui::get_mouse_position()+12*guiscale());

    if (m_tipWindow->pointAt(arrowAlign, target)) {
      m_tipWindow->openWindow();
    }
    else {
      // No enough room for the tooltip
      m_tipWindow.reset();
    }
  }
  m_timer->stop();
}

// TipWindow

TipWindow::TipWindow(const std::string& text)
  : PopupWindow(text, ClickBehavior::CloseOnClickInOtherWindow)
  , m_arrowAlign(0)
  , m_closeOnKeyDown(true)
{
  setTransparent(true);

  makeFixed();
  initTheme();
}

void TipWindow::setCloseOnKeyDown(bool state)
{
  m_closeOnKeyDown = state;
}

bool TipWindow::pointAt(int arrowAlign, const gfx::Rect& target)
{
  m_target = target;
  m_arrowAlign = arrowAlign;

  remapWindow();

  int x = target.x;
  int y = target.y;
  int w = bounds().w;
  int h = bounds().h;

  int trycount = 0;
  for (; trycount < 4; ++trycount) {
    switch (arrowAlign) {
      case TOP | LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y + m_target.h;
        break;
      case TOP | RIGHT:
        x = m_target.x - w;
        y = m_target.y + m_target.h;
        break;
      case BOTTOM | LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y - h;
        break;
      case BOTTOM | RIGHT:
        x = m_target.x - w;
        y = m_target.y - h;
        break;
      case TOP:
        x = m_target.x + m_target.w/2 - w/2;
        y = m_target.y + m_target.h;
        break;
      case BOTTOM:
        x = m_target.x + m_target.w/2 - w/2;
        y = m_target.y - h;
        break;
      case LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y + m_target.h/2 - h/2;
        break;
      case RIGHT:
        x = m_target.x - w;
        y = m_target.y + m_target.h/2 - h/2;
        break;
    }

    x = MID(0, x, ui::display_w()-w);
    y = MID(0, y, ui::display_h()-h);

    if (m_target.intersects(gfx::Rect(x, y, w, h))) {
      switch (trycount) {
        case 0:
        case 2:
          // Switch position
          if (arrowAlign & (TOP | BOTTOM)) arrowAlign ^= TOP | BOTTOM;
          if (arrowAlign & (LEFT | RIGHT)) arrowAlign ^= LEFT | RIGHT;
          break;
        case 1:
          // Rotate positions
          if (arrowAlign & (TOP | LEFT)) arrowAlign ^= TOP | LEFT;
          if (arrowAlign & (BOTTOM | RIGHT)) arrowAlign ^= BOTTOM | RIGHT;
          break;
      }
    }
    else {
      m_arrowAlign = arrowAlign;
      positionWindow(x, y);
      break;
    }
  }

  return (trycount < 4);
}

bool TipWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage:
      if (m_closeOnKeyDown &&
          static_cast<KeyMessage*>(msg)->scancode() < kKeyFirstModifierScancode)
        closeWindow(NULL);
      break;

  }

  return PopupWindow::onProcessMessage(msg);
}

void TipWindow::onSizeHint(SizeHintEvent& ev)
{
  ScreenGraphics g;
  g.setFont(font());
  Size resultSize =
    g.fitString(text(),
                (clientBounds() - border()).w,
                align());

  resultSize.w += border().width();
  resultSize.h += border().height();

  if (!children().empty()) {
    Size maxSize(0, 0);
    Size reqSize;

    for (auto child : children()) {
      reqSize = child->sizeHint();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }

    resultSize.w = MAX(resultSize.w, maxSize.w + border().width());
    resultSize.h += maxSize.h;
  }

  ev.setSizeHint(resultSize);
}

void TipWindow::onInitTheme(InitThemeEvent& ev)
{
  Window::onInitTheme(ev);

  setBorder(
    gfx::Border(6 * guiscale(),
                6 * guiscale(),
                6 * guiscale(),
                7 * guiscale()));
}

void TipWindow::onPaint(PaintEvent& ev)
{
  theme()->paintTooltip(ev);
}

} // namespace ui
