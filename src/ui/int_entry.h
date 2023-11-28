// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/entry.h"
#include "ui/slider.h"

namespace ui {

  class CloseEvent;
  class PopupWindow;

  class IntEntry : public Entry {
  public:
    IntEntry(int min, int max, SliderDelegate* sliderDelegate = nullptr);
    ~IntEntry();

    int getValue() const;
    void setValue(int value);
    int min() {return m_min;}
    int max() {return m_max;}
    void setMin(int value) {m_min = value; m_slider.setRange(m_min, m_max);}
    void setMax(int value) {m_max = value; m_slider.setRange(m_min, m_max);}

  protected:
    bool onProcessMessage(Message* msg) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onChange() override;
    bool showsKeyboard() const override {return false;}

    // New events
    virtual void onValueChange();

  private:
    void openPopup();
    void closePopup();
    void onChangeSlider();
    void onPopupClose(CloseEvent& ev);
    void removeSlider();

    int m_min;
    int m_max;
    Slider m_slider;
    PopupWindow* m_popupWindow;
    bool m_changeFromSlider;
  };

} // namespace ui
