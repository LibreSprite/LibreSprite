// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "she/display.h"

namespace she {

  class Alleg4Display : public Display {
  public:
    Alleg4Display(int width, int height, int scale);
    ~Alleg4Display();

    void dispose() override;
    int width() const override;
    int height() const override;
    int originalWidth() const override;
    int originalHeight() const override;
    void setOriginalWidth(int width);
    void setOriginalHeight(int height);
    int scale() const override;
    void setScale(int scale) override;
    void recreateSurface();
    Surface* getSurface() override;
    void flip(const gfx::Rect& bounds) override;
    void maximize() override;
    bool isMaximized() const override;
    bool isMinimized() const override;
    void setTitleBar(const std::string& title) override;
    NativeCursor nativeMouseCursor() override;
    bool setNativeMouseCursor(NativeCursor cursor) override;
    void setMousePosition(const gfx::Point& position) override;
    void captureMouse() override;
    void releaseMouse() override;
    std::string getLayout() override;
    void setLayout(const std::string& layout) override;
    void* nativeHandle() override;

  private:
    Surface* m_surface;
    int m_scale;
    NativeCursor m_nativeCursor;
    int m_restoredWidth;
    int m_restoredHeight;
  };

  extern Alleg4Display* unique_display;

} // namespace she
