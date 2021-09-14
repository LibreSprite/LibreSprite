// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"
#include "she/native_cursor.h"
#include "she/x11/window.h"

#include <string>

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public X11Window {
public:
  enum class Backend { NONE, GL };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  int scale() const;
  void setScale(int scale);
  void setVisible(bool visible);
  void maximize();
  bool isMaximized() const;
  bool isMinimized() const;
  gfx::Size clientSize() const;
  gfx::Size restoredSize() const;
  void captureMouse();
  void releaseMouse();
  void setMousePosition(const gfx::Point& position);
  bool setNativeMouseCursor(NativeCursor cursor);
  void updateWindow(const gfx::Rect& bounds);
  std::string getLayout() { return ""; }
  void setLayout(const std::string& layout) { }

private:
  void onExpose() override;

  gfx::Size m_clientSize;
  int m_scale;

  DISABLE_COPYING(SkiaWindow);
};

} // namespace she
