// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

namespace app {
namespace tools {

class IntertwineNone : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    for (int c=0; c<stroke.size(); ++c)
      doPointshapePoint(stroke[c].x, stroke[c].y, stroke[c].pressure, loop);
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsLines : public Intertwine {
public:
  bool snapByAngle() override { return true; }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() == 0)
      return;

    if (stroke.size() == 1) {
      doPointshapePoint(stroke[0].x, stroke[0].y, stroke[0].pressure, loop);
    }
    else if (stroke.size() >= 2) {
      for (int c=0; c+1<stroke.size(); ++c) {
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        auto p1 = stroke[c].pressure;
        int x2 = stroke[c+1].x;
        int y2 = stroke[c+1].y;
        auto p2 = stroke[c+1].pressure;

        algo_line_float(x1, y1,
                        x2, y2,
                        [&](int x, int y, float f){
                          doPointshapePoint(x, y, p1*(1-f) + p2*f, loop);
                        });
      }
    }

    // Closed shape (polygon outline)
    if (loop->getFilled()) {
      auto& first = stroke[0];
      auto& last = stroke[stroke.size() - 1];
      auto p1 = first.pressure;
      auto p2 = last.pressure;
      algo_line_float(first.x, first.y,
                      last.x, last.y,
                      [&](int x, int y, float f){
                        doPointshapePoint(x, y, p1*(1-f) + p2*f, loop);
                      });
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }

    // Contour
    joinStroke(loop, stroke);

    // Fill content
    doc::algorithm::polygon(stroke, [&](int x, int y, int x2){
      doPointshapeHline(x, y, x2, stroke[0].pressure, loop);
    });
  }
};

class IntertwineAsRectangles : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() == 0)
      return;

    if (stroke.size() == 1) {
      doPointshapePoint(stroke[0].x, stroke[0].y, stroke[0].pressure, loop);
    }
    else if (stroke.size() >= 2) {
      for (int c=0; c+1<stroke.size(); ++c) {
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        auto p1 = stroke[c].pressure;
        int x2 = stroke[c+1].x;
        int y2 = stroke[c+1].y;
        // auto p2 = stroke[c+1].pressure; // to-do: pressure tween
        int y;

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        doPointshapeLine(x1, y1, x2, y1, p1, loop);
        doPointshapeLine(x1, y2, x2, y2, p1, loop);

        for (y=y1; y<=y2; y++) {
          doPointshapePoint(x1, y, p1, loop);
          doPointshapePoint(x2, y, p1, loop);
        }
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() < 2) {
      joinStroke(loop, stroke);
      return;
    }

    for (int c=0; c+1<stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      auto p1 = stroke[c].pressure;
      int x2 = stroke[c+1].x;
      int y2 = stroke[c+1].y;
      int y;

      if (x1 > x2) std::swap(x1, x2);
      if (y1 > y2) std::swap(y1, y2);

      for (y=y1; y<=y2; y++)
        doPointshapeLine(x1, y, x2, y, p1, loop);
    }
  }
};

class IntertwineAsEllipses : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() == 0)
      return;

    if (stroke.size() == 1) {
      doPointshapePoint(stroke[0].x, stroke[0].y, stroke[0].pressure, loop);
    }
    else if (stroke.size() >= 2) {
      for (int c=0; c+1<stroke.size(); ++c) {
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        auto p1 = stroke[c].pressure;
        int x2 = stroke[c+1].x;
        int y2 = stroke[c+1].y;

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        algo_ellipse(x1, y1, x2, y2, [&](int x, int y){
          doPointshapePoint(x, y, p1, loop);
        });
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() < 2) {
      joinStroke(loop, stroke);
      return;
    }

    for (int c=0; c+1<stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      auto p1 = stroke[c].pressure;
      int x2 = stroke[c+1].x;
      int y2 = stroke[c+1].y;

      if (x1 > x2) std::swap(x1, x2);
      if (y1 > y2) std::swap(y1, y2);

      algo_ellipsefill(x1, y1, x2, y2, [&](int x, int y, int x2){
        doPointshapeHline(x, y, x2, p1, loop);
      });
    }
  }
};

class IntertwineAsBezier : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() == 0)
      return;

    for (int c=0; c<stroke.size(); c += 4) {
      if (stroke.size()-c == 1) {
        doPointshapePoint(stroke[c].x, stroke[c].y, stroke[c].pressure, loop);
      }
      else if (stroke.size()-c == 2) {
        algo_line(stroke[c].x, stroke[c].y,
                  stroke[c+1].x, stroke[c+1].y, [&](int x, int y){doPointshapePoint(x, y, stroke[c].pressure, loop);});
      }
      else if (stroke.size()-c == 3) {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y,
                    [&](int x, int y, int x2, int y2){
                      doPointshapeLine(x, y, x2, y2, stroke[c].pressure, loop);
                    });
      }
      else {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y,
                    stroke[c+3].x, stroke[c+3].y,
                    [&](int x, int y, int x2, int y2){
                      doPointshapeLine(x, y, x2, y2, stroke[c].pressure, loop);
                    });
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsPixelPerfect : public Intertwine {
  static void pixelPerfectLine(int x, int y, float pressure, Stroke* stroke) {
    Stroke::Point newPoint{x, y, pressure};
    if (stroke->empty() ||
        stroke->lastPoint() != newPoint) {
      stroke->addPoint(newPoint);
    }
  }

  Stroke m_pts;

public:
  void prepareIntertwine() override {
    m_pts.reset();
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() == 0)
      return;
    else if (m_pts.empty() && stroke.size() == 1) {
      m_pts = stroke;
    }
    else {
      for (int c=0; c+1<stroke.size(); ++c) {
        auto& p1 = stroke[c];
        auto& p2 = stroke[c+1];
        algo_line_float(
          p1.x, p1.y,
          p2.x, p2.y,
          [&](int x, int y, float f){
            pixelPerfectLine(x, y, p1.pressure*(1-f) + p2.pressure*f, &m_pts);
          });
      }
    }

    for (int c=0; c<m_pts.size(); ++c) {
      // We ignore a pixel that is between other two pixels in the
      // corner of a L-like shape.
      if (c > 0 && c+1 < m_pts.size()
        && (m_pts[c-1].x == m_pts[c].x || m_pts[c-1].y == m_pts[c].y)
        && (m_pts[c+1].x == m_pts[c].x || m_pts[c+1].y == m_pts[c].y)
        && m_pts[c-1].x != m_pts[c+1].x
        && m_pts[c-1].y != m_pts[c+1].y) {
        ++c;
      }

      doPointshapePoint(m_pts[c].x, m_pts[c].y, m_pts[c].pressure, loop);
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }

    // Contour
    joinStroke(loop, stroke);

    // Fill content
    doc::algorithm::polygon(stroke, [&](int x, int y, int x2){
      doPointshapeHline(x, y, x2, 1.0f, loop);
    });
  }
};

} // namespace tools
} // namespace app
