// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/connection.h"
#include "ui/view.h"

namespace app {
  class Editor;

  class EditorView : public ui::View {
  public:
    enum Type { CurrentEditorMode, AlwaysSelected };

    enum Method { KeepOrigin, KeepCenter };
    static void SetScrollUpdateMethod(Method method);

    EditorView(Type type);

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onSetViewScroll(const gfx::Point& pt) override;
    void onScrollRegion(ui::ScrollRegionEvent& ev) override;
    void onScrollChange() override;

  private:
    Editor* editor();
    void setupScrollbars();

    Type m_type;
    base::ScopedConnection m_scrollSettingsConn;
    static Method g_scrollUpdateMethod;
  };

} // namespace app
