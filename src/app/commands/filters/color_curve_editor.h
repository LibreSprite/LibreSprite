// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/signal.h"
#include "gfx/point.h"
#include "ui/widget.h"

namespace filters {
  class ColorCurve;
}

namespace app {
  using namespace filters;

  class ColorCurveEditor : public ui::Widget {
  public:
    ColorCurve* getCurve() const { return m_curve; }
    void setCurve(ColorCurve* curve) { m_curve = curve; }

    void setViewBounds(const gfx::Rect& rect) { m_viewBounds = rect; }

    base::Signal0<void> CurveEditorChange;

  protected:
    ColorCurveEditor();
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    gfx::Point* getClosestPoint(const gfx::Point& viewPt);
    bool editNodeManually(gfx::Point& viewPt);
    gfx::Point viewToClient(const gfx::Point& viewPt);
    gfx::Point screenToView(const gfx::Point& screenPt);
    gfx::Point clientToView(const gfx::Point& clientPt);
    void addPoint(const gfx::Point& viewPoint);
    void removePoint(gfx::Point* viewPoint);

    ColorCurve* m_curve = nullptr;
    int m_status;
    gfx::Rect m_viewBounds;
    gfx::Point* m_hotPoint = nullptr;
    gfx::Point* m_editPoint = nullptr;
  };

} // namespace app
