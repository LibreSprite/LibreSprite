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
    ColorCurveEditor(ColorCurve* curve, const gfx::Rect& viewBounds);

    ColorCurve* getCurve() const { return m_curve; }

    base::Signal0<void> CurveEditorChange;

  protected:
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

    ColorCurve* m_curve;
    int m_status;
    gfx::Rect m_viewBounds;
    gfx::Point* m_hotPoint;
    gfx::Point* m_editPoint;
  };

} // namespace app
