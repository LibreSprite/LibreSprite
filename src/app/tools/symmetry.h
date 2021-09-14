// Aseprite    | Copyright (C) 2015  David Capello
// LibreSprite | Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/tools/stroke.h"

#include <vector>

namespace app {
  namespace tools {

    class ToolLoop;

    // This class controls user input.
    class Symmetry {
    public:
      virtual ~Symmetry() { }

      // The "stroke" must be relative to the sprite origin.
      virtual void generateStrokes(const Stroke& stroke, Strokes& strokes, ToolLoop* loop) = 0;
    };

    class HorizontalSymmetry : public Symmetry {
    public:
      HorizontalSymmetry(int x) : m_x(x) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x;
    };

    class VerticalSymmetry : public Symmetry {
    public:
      VerticalSymmetry(int y) : m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_y;
    };

    class DoubleSymmetry : public Symmetry {
    public:
      DoubleSymmetry(int x, int y) : m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x;
      int m_y;
    };

  } // namespace tools
} // namespace app
