// SHE library
// Copyright (C) 2012-2016  David Capello
// Copyright (C) 2021       LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>
#include "gfx/rect.h"

namespace she {

  enum class FontType {
    kUnknown,
    kSpriteSheet,
    kTrueType,
  };

  class Font {
  public:
    virtual ~Font() { }
    virtual void dispose() = 0;
    virtual FontType type() = 0;
    virtual int height() const = 0;
    virtual gfx::Rect charBounds(int chr) const = 0;
    virtual int charWidth(int chr) const = 0;
    virtual int textLength(const std::string& str) const = 0;
    virtual bool isScalable() const = 0;
    virtual void setSize(int size) = 0;
    virtual void setAntialias(bool antialias) = 0;
  };

} // namespace she
