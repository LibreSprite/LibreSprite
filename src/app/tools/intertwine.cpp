// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/intertwine.h"

#include "app/tools/point_shape.h"
#include "app/tools/stroke.h"
#include "app/tools/symmetry.h"
#include "app/tools/tool_loop.h"
#include "doc/algo.h"

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;

void Intertwine::doPointshapePoint(int x, int y, float pressure, ToolLoop* loop)
{
  Symmetry* symmetry = loop->getSymmetry();
  if (symmetry) {
    // Convert the point to the sprite position so we can apply the
    // symmetry transformation.
    Stroke main_stroke;
    main_stroke.addPoint({x, y, pressure});

    Strokes strokes;
    symmetry->generateStrokes(main_stroke, strokes, loop);
    for (const auto& stroke : strokes) {
      // We call transformPoint() moving back each point to the cel
      // origin.
      loop->getPointShape()->transformPoint(
        loop, stroke[0].x, stroke[0].y, pressure);
    }
  }
  else {
    loop->getPointShape()->transformPoint(loop, x, y, pressure);
  }
}

void Intertwine::doPointshapeHline(int x1, int y, int x2, float pressure, ToolLoop* loop)
{
  algo_line(x1, y, x2, y, [&](auto x, auto y){doPointshapePoint(x, y, pressure, loop);});
}

void Intertwine::doPointshapeLine(int x1, int y1, int x2, int y2, float pressure, ToolLoop* loop)
{
  algo_line(x1, y1, x2, y2, [&](auto x, auto y){doPointshapePoint(x, y, pressure, loop);});
}

} // namespace tools
} // namespace app
