// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/base.h"
#include "ui/popup_window.h"
#include "ui/timer.h"
#include "ui/window.h"

#include <unordered_map>

namespace ui {

  class TipWindow;

  class TooltipManager : public Widget {
  public:
    TooltipManager();

    void addTooltipFor(std::shared_ptr<Widget> widget, const std::string& text, int arrowAlign = 0);
    void addTooltipFor(Widget* widget, const std::string& text, int arrowAlign = 0);
    void removeTooltipFor(Widget* widget);

  protected:
    bool onProcessMessage(Message* msg) override;

  private:
    void onTick();

    struct TipInfo {
      std::string text;
      int arrowAlign;

      TipInfo() { }
      TipInfo(const std::string& text, int arrowAlign)
        : text(text), arrowAlign(arrowAlign) {
      }
    };

    typedef std::unordered_map<Widget*, TipInfo> Tips;
    Tips m_tips;                      // All tips.
    std::unique_ptr<TipWindow> m_tipWindow; // Frame to show tooltips.
    inject<Timer> m_timer{nullptr};         // Timer to control the tooltip delay.
    struct {
      Widget* widget;
      TipInfo tipInfo;
    } m_target;
  };

  class TipWindow : public PopupWindow {
  public:
    TipWindow(const std::string& text = "");

    int arrowAlign() const { return m_arrowAlign; }
    const gfx::Rect& target() const { return m_target; }

    void setCloseOnKeyDown(bool state);

    // Returns false there is no enough screen space to show the
    // window.
    bool pointAt(int arrowAlign, const gfx::Rect& target);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onInitTheme(InitThemeEvent& ev) override;
    void onPaint(PaintEvent& ev) override;

  private:
    int m_arrowAlign;
    gfx::Rect m_target;
    bool m_closeOnKeyDown;
  };

} // namespace ui
