// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/slider.h"

#include "she/font.h"
#include "ui/events/size_hint_event.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cstdio>
#include <cstdlib>

namespace ui {

static int slider_press_x;
static int slider_press_value;
static bool slider_press_left;

Slider::Slider(int min, int max, int value, SliderDelegate* delegate)
  : Widget(kSliderWidget)
  , m_min(min)
  , m_max(max)
  , m_value(MID(min, value, max))
  , m_readOnly(false)
  , m_delegate(delegate)
{
  this->setFocusStop(true);
  initTheme();
}

void Slider::setRange(int min, int max)
{
  m_min = min;
  m_max = max;
  m_value = MID(min, m_value, max);

  invalidate();
}

void Slider::setValue(int value)
{
  int old_value = m_value;

  m_value = MID(m_min, value, m_max);

  if (m_value != old_value)
    invalidate();

  // It DOES NOT emit CHANGE signal! to avoid recursive calls.
}

void Slider::getSliderThemeInfo(int* min, int* max, int* value) const
{
  if (min) *min = m_min;
  if (max) *max = m_max;
  if (value) *value = m_value;
}

std::string Slider::convertValueToText(int value) const
{
  if (m_delegate)
    return m_delegate->onGetTextFromValue(value);
  else {
    char buf[128];
    std::sprintf(buf, "%d", value);
    return buf;
  }
}

int Slider::convertTextToValue(const std::string& text) const
{
  if (m_delegate)
    return m_delegate->onGetValueFromText(text);
  else {
    return std::strtol(text.c_str(), NULL, 10);
  }
}

bool Slider::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      if (isEnabled())
        invalidate();
      break;

    case kMouseDownMessage:
      if (!isEnabled() || isReadOnly())
        return true;

      setSelected(true);
      captureMouse();

      {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        slider_press_x = mousePos.x;
        slider_press_value = m_value;
        slider_press_left = static_cast<MouseMessage*>(msg)->left();
      }

      setupSliderCursor();

      // Fall through

    case kMouseMoveMessage:
      if (hasCapture()) {
        int value, accuracy, range;
        gfx::Rect rc = childrenBounds();
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        range = m_max - m_min + 1;

        // With left click
        if (slider_press_left) {
          value = m_min + range * (mousePos.x - rc.x) / rc.w;
        }
        // With right click
        else {
          accuracy = MID(1, rc.w / range, rc.w);

          value = slider_press_value +
            (mousePos.x - slider_press_x) / accuracy;
        }

        value = MID(m_min, value, m_max);

        if (m_value != value) {
          setValue(value);
          onChange();
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        setSelected(false);
        releaseMouse();
        setupSliderCursor();

        onSliderReleased();
      }
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;

    case kKeyDownMessage:
      if (hasFocus() && !isReadOnly()) {
        int min = m_min;
        int max = m_max;
        int value = m_value;

        switch (static_cast<KeyMessage*>(msg)->scancode()) {
          case kKeyLeft:     value = MAX(value-1, min); break;
          case kKeyRight:    value = MIN(value+1, max); break;
          case kKeyPageDown: value = MAX(value-(max-min+1)/4, min); break;
          case kKeyPageUp:   value = MIN(value+(max-min+1)/4, max); break;
          case kKeyHome:     value = min; break;
          case kKeyEnd:      value = max; break;
          default:
            goto not_used;
        }

        if (m_value != value) {
          setValue(value);
          onChange();
        }

        return true;
      }
      break;

    case kMouseWheelMessage:
      if (isEnabled() && !isReadOnly()) {
        int value = m_value
          + static_cast<MouseMessage*>(msg)->wheelDelta().x
          - static_cast<MouseMessage*>(msg)->wheelDelta().y;

        value = MID(m_min, value, m_max);

        if (m_value != value) {
          this->setValue(value);
          onChange();
        }
        return true;
      }
      break;

    case kSetCursorMessage:
      setupSliderCursor();
      return true;
  }

not_used:;
  return Widget::onProcessMessage(msg);
}

void Slider::onSizeHint(SizeHintEvent& ev)
{
  int min_w = font()->textLength(convertValueToText(m_min));
  int max_w = font()->textLength(convertValueToText(m_max));

  int w = MAX(min_w, max_w);
  int h = textHeight();

  w += border().width();
  h += border().height();

  ev.setSizeHint(w, h);
}

void Slider::onPaint(PaintEvent& ev)
{
  theme()->paintSlider(ev);
}

void Slider::onChange()
{
  Change(); // Emit Change signal
}

void Slider::onSliderReleased()
{
  SliderReleased();
}

void Slider::setupSliderCursor()
{
  if (hasCapture()) {
    if (slider_press_left)
      set_mouse_cursor(kArrowCursor);
    else
      set_mouse_cursor(kSizeWECursor);
  }
  else
    set_mouse_cursor(kArrowCursor);
}

} // namespace ui
