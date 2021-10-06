// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/tools/active_tool_observer.h"
#include "base/connection.h"
#include "gfx/point.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <map>

namespace ui {
  class CloseEvent;
  class PopupWindow;
  class TipWindow;
}

namespace app {
  namespace tools {
    class Tool;
    class ToolGroup;
  }

  // Class to show selected tools for each tool (vertically)
  class ToolBar : public ui::Widget
                , public tools::ActiveToolObserver {
    static ToolBar* m_instance;
  public:
    static ToolBar* instance() { return m_instance; }

    static const int NoneIndex = -1;
    static const int PreviewVisibilityIndex = -2;

    ToolBar();
    ~ToolBar();

    bool isToolVisible(tools::Tool* tool);
    void selectTool(tools::Tool* tool);

    void openTipWindow(tools::ToolGroup* tool_group, tools::Tool* tool);
    void closeTipWindow();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    int getToolGroupIndex(tools::ToolGroup* group);
    void openPopupWindow(int group_index, tools::ToolGroup* group);
    gfx::Rect getToolGroupBounds(int group_index);
    gfx::Point getToolPositionInGroup(int group_index, tools::Tool* tool);
    void openTipWindow(int group_index, tools::Tool* tool);
    void onClosePopup();

    // ActiveToolObserver impl
    void onSelectedToolChange(tools::Tool* tool) override;

    // What tool is selected for each tool-group
    std::map<const tools::ToolGroup*, tools::Tool*> m_selectedInGroup;

    // Index of the tool group or special button highlighted.
    int m_hotIndex = NoneIndex;

    // What tool has the mouse above
    tools::Tool* m_hotTool = nullptr;

    // True if the popup-window must be opened when a tool-button is hot
    bool m_openOnHot = false;

    // True if the last MouseDown opened the popup. This is used to
    // close the popup with a second MouseUp event.
    bool m_openedRecently = false;

    // Window displayed to show a tool-group
    ui::PopupWindow* m_popupWindow = nullptr;
    class ToolStrip;
    ToolStrip* m_currentStrip = nullptr;

    // Tool-tip window
    ui::TipWindow* m_tipWindow = nullptr;

    inject<ui::Timer> m_tipTimer = ui::Timer::create(300, *this);
    bool m_tipOpened = false;

    base::Connection m_closeConn;
  };

} // namespace app
