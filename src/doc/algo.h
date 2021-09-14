// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/fwd.h"
#include "doc/algorithm/hline.h"

namespace doc {

  class Image;

  typedef void (*AlgoPixel)(int x, int y, void *data);
  typedef void (*AlgoLine)(int x1, int y1, int x2, int y2, void *data);

  void algo_line(int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);
  void algo_ellipse(int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);
  void algo_ellipsefill(int x1, int y1, int x2, int y2, void *data, AlgoHLine proc);

  void algo_spline(double x0, double y0, double x1, double y1,
                   double x2, double y2, double x3, double y3,
                   void *data, AlgoLine proc);
  double algo_spline_get_y(double x0, double y0, double x1, double y1,
                           double x2, double y2, double x3, double y3,
                           double x);
  double algo_spline_get_tan(double x0, double y0, double x1, double y1,
                             double x2, double y2, double x3, double y3,
                             double in_x);

} // namespace doc
