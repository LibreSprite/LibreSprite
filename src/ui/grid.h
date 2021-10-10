// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ui/widget.h"

#include <vector>

namespace ui {

  class Grid : public Widget {
  public:
    struct Info {
      int col = 0, row = 0;
      int hspan = 0, vspan = 0;
      int grid_cols = 0, grid_rows = 0;
    };

    Grid(int columns = 1, bool same_width_columns = true);

    void setSameWidthColumns(bool value) {m_sameWidthColumns = value;}
    void setColumns(int columns);

    void addChildInCell(Widget* child, int hspan, int vspan, int align);
    void addChildInCell(std::shared_ptr<Widget> child, int hspan, int vspan, int align);
    Info getChildInfo(Widget* child);

  protected:
    // Events
    void onResize(ResizeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;

  private:
    struct Cell {
      Cell* parent = nullptr;
      Widget* child = nullptr;
      int hspan = 0;
      int vspan = 0;
      int align = 0;
      int w = 0, h = 0;
    };

    struct Strip {
      int size = 0;
      int expand_count = 0;
    };

    void sumStripSize(const std::vector<Strip>& strip, int& size);
    void calculateCellSize(int start, int span, const std::vector<Strip>& strip, int& size);
    void calculateSize();
    void calculateStripSize(std::vector<Strip>& colstrip,
                            std::vector<Strip>& rowstrip, int align);
    void expandStrip(std::vector<Strip>& colstrip,
                     std::vector<Strip>& rowstrip,
                     void (Grid::*incCol)(int, int));
    void distributeSize(const gfx::Rect& rect);
    void distributeStripSize(std::vector<Strip>& colstrip,
                             int rect_size, int border_size, bool same_width);
    bool putWidgetInCell(Widget* child, int hspan, int vspan, int align);
    void expandRows(int rows);
    void incColSize(int col, int size);
    void incRowSize(int row, int size);

    bool m_sameWidthColumns;
    std::vector<Strip> m_colStrip;
    std::vector<Strip> m_rowStrip;
    std::vector<std::vector<Cell> > m_cells;
  };

} // namespace ui
