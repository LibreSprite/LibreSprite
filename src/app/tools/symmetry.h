// Aseprite    | Copyright (C) 2015       David Capello
// LibreSprite | Copyright (C) 2021-2026  LibreSprite contributors
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

    // Reflects across the "/" diagonal (slope -1) through (x,y).
    class Diagonal45Symmetry : public Symmetry {
    public:
      Diagonal45Symmetry(int x, int y) : m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x, m_y;
    };

    // Reflects across the "\" diagonal (slope +1) through (x,y).
    class Diagonal135Symmetry : public Symmetry {
    public:
      Diagonal135Symmetry(int x, int y) : m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x, m_y;
    };

    // Point (180deg) reflection about (x,y): equivalent to mirroring
    // horizontally and then flipping that mirrored copy vertically, but
    // produces a single rotated companion stroke rather than a 4-way tile.
    class Rotational180Symmetry : public Symmetry {
    public:
      Rotational180Symmetry(int x, int y) : m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x, m_y;
    };

    // Full 4-way dihedral symmetry about (x,y): each of the 4 quadrants is
    // the previous one (going top-left, top-right, bottom-right,
    // bottom-left) rotated 90deg clockwise about the origin.
    class Rotational90Symmetry : public Symmetry {
    public:
      Rotational90Symmetry(int x, int y) : m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_x, m_y;
    };

    // Combines any subset of the HORIZONTAL/VERTICAL/DIAGONAL_45/DIAGONAL_135
    // app::gen::SymmetryMode bit flags, doubling the stroke set for each
    // active axis (so e.g. HORIZONTAL|VERTICAL produces the 4 strokes of the
    // former "BOTH" mode).
    class CompositeSymmetry : public Symmetry {
    public:
      CompositeSymmetry(int flags, int x, int y) : m_flags(flags), m_x(x), m_y(y) { }
      void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                           ToolLoop* loop) override;
    private:
      int m_flags;
      int m_x, m_y;
    };

  } // namespace tools
} // namespace app
