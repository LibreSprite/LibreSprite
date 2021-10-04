// SHE library
// Copyright (C) 2016  David Capello
// Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "ft/face.h"
#include "ft/lib.h"
#include "she/font.h"

namespace she {
  class Font;

  class FreeTypeFont : public Font {
  public:
    FreeTypeFont(const char* filename, int height);
    ~FreeTypeFont();

    bool isValid() const;
    void dispose() override;
    FontType type() override;
    int height() const override;
    gfx::Rect charBounds(int chr) const override;
    int charWidth(int chr) const override;
    int textLength(const std::string& str) const override;
    bool isScalable() const override;
    void setSize(int size) override;
    void setAntialias(bool antialias) override;

    ft::Face& face() { return m_face; }

  private:
    mutable ft::Lib m_ft;
    mutable ft::Face m_face;
  };

  FreeTypeFont* loadFreeTypeFont(const char* filename, int height);

} // namespace she
