// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/brush_pattern.h"
#include "doc/brush_type.h"
#include "doc/color.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {

  class Brush {
  public:
    static const int kMinBrushSize = 1;
    static const int kMaxBrushSize = 64;

    enum class ImageColor { MainColor, BackgroundColor };

    Brush();
    Brush(BrushType type, int size, int angle);
    Brush(const Brush& brush);
    ~Brush();

    BrushType type() const { return m_type; }
    int size() const { return m_size; }
    int angle() const { return m_angle; }
    Image* image();
    int gen() const { return m_gen; }
    Image* image(float scale);

    BrushPattern pattern() const { return m_pattern; }
    gfx::Point patternOrigin() const { return m_patternOrigin; }

    const gfx::Rect& bounds() const { return m_bounds; }
    const gfx::Rect& scaledBounds() const { return m_scaledBounds; }

    void setType(BrushType type);
    void setSize(int size);
    void setAngle(int angle);
    void setImage(const Image* image);
    void setImageColor(ImageColor imageColor, color_t color);
    void setPattern(BrushPattern pattern) {
      m_pattern = pattern;
    }
    void setPatternOrigin(const gfx::Point& patternOrigin) {
      m_patternOrigin = patternOrigin;
    }

  private:
    void clean();
    void regenerate();

    BrushType m_type;                     // Type of brush
    int m_size;                           // Size (diameter)
    int m_angle;                          // Angle in degrees 0-360
    ImageRef m_image;                     // Image of the brush
    gfx::Rect m_bounds;
    gfx::Rect m_scaledBounds;
    BrushPattern m_pattern;               // How the image should be replicated
    gfx::Point m_patternOrigin;           // From what position the brush was taken
    int m_gen;
    int m_genSize{};

    // Extra data used for setImageColor()
    std::shared_ptr<Image> m_backupImage; // Backup image to avoid losing original brush colors/pattern
    std::unique_ptr<color_t> m_mainColor; // Main image brush color (nullptr if it wasn't specified)
    std::unique_ptr<color_t> m_bgColor;   // Background color (nullptr if it wasn't specified)
  };

  typedef base::SharedPtr<Brush> BrushRef;

} // namespace doc
