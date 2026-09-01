// Aseprite    | Copyright (C) 2015       David Capello
// LibreSprite | Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/symmetry.h"

#include "app/pref/preferences.h"
#include "app/tools/stroke.h"
#include "app/tools/tool_loop.h"
#include "doc/brush.h"

namespace app {
namespace tools {

void HorizontalSymmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                         ToolLoop* loop)
{
  int adjust = (loop->getBrush()->bounds().w % 2);

  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint({m_x - (pt.x - m_x + adjust), pt.y, pt.pressure});
  strokes.push_back(stroke2);
}

void VerticalSymmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                       ToolLoop* loop)
{
  int adjust = (loop->getBrush()->bounds().h % 2);

  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint({pt.x, m_y - (pt.y - m_y + adjust), pt.pressure});
  strokes.push_back(stroke2);
}

void Diagonal45Symmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                         ToolLoop* loop)
{
  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint({m_x - (pt.y - m_y), m_y - (pt.x - m_x), pt.pressure});
  strokes.push_back(stroke2);
}

void Diagonal135Symmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                          ToolLoop* loop)
{
  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint({m_x + (pt.y - m_y), m_y + (pt.x - m_x), pt.pressure});
  strokes.push_back(stroke2);
}

void Rotational180Symmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                          ToolLoop* loop)
{
  int adjustX = (loop->getBrush()->bounds().w % 2);
  int adjustY = (loop->getBrush()->bounds().h % 2);

  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint({m_x - (pt.x - m_x + adjustX), m_y - (pt.y - m_y + adjustY), pt.pressure});
  strokes.push_back(stroke2);
}

void Rotational90Symmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                         ToolLoop* loop)
{
  strokes.push_back(mainStroke);

  // Each of the 3 companion strokes is the previous quadrant's stroke
  // rotated 90deg clockwise about (m_x, m_y): (dx,dy) -> (-dy,dx).
  Stroke prev = mainStroke;
  for (int quadrant = 0; quadrant < 3; ++quadrant) {
    Stroke stroke;
    for (const auto& pt : prev) {
      int dx = pt.x - m_x;
      int dy = pt.y - m_y;
      stroke.addPoint({m_x - dy, m_y + dx, pt.pressure});
    }
    strokes.push_back(stroke);
    prev = stroke;
  }
}

void CompositeSymmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                        ToolLoop* loop)
{
  strokes.push_back(mainStroke);

  auto reflect = [&](Symmetry& axis) {
    Strokes reflected;
    for (const Stroke& stroke : strokes) {
      Strokes tmp;
      axis.generateStrokes(stroke, tmp, loop);
      // Each of the axis classes above always pushes the input stroke
      // first, followed by its mirrored copy.
      reflected.push_back(tmp[1]);
    }
    strokes.insert(strokes.end(), reflected.begin(), reflected.end());
  };

  if (m_flags & (int)app::gen::SymmetryMode::HORIZONTAL) {
    HorizontalSymmetry axis(m_x);
    reflect(axis);
  }
  if (m_flags & (int)app::gen::SymmetryMode::VERTICAL) {
    VerticalSymmetry axis(m_y);
    reflect(axis);
  }
  if (m_flags & (int)app::gen::SymmetryMode::DIAGONAL_45) {
    Diagonal45Symmetry axis(m_x, m_y);
    reflect(axis);
  }
  if (m_flags & (int)app::gen::SymmetryMode::DIAGONAL_135) {
    Diagonal135Symmetry axis(m_x, m_y);
    reflect(axis);
  }
}

} // namespace tools
} // namespace app
