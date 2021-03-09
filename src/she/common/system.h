// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "she/common/freetype_font.h"
#include "she/common/sprite_sheet_font.h"
#include "she/system.h"

namespace she {

#ifdef __APPLE__
Logger* getOsxLogger();
#endif

class CommonSystem : public System {
public:
  void dispose() override {
    delete this;
  }

  Logger* logger() override {
#ifdef __APPLE__
    return getOsxLogger();
#else
    return nullptr;
#endif
  }

  Font* loadSpriteSheetFont(const char* filename, int scale) override {
    Surface* sheet = loadRgbaSurface(filename);
    Font* font = nullptr;
    if (sheet) {
      sheet->applyScale(scale);
      font = SpriteSheetFont::fromSurface(sheet);
    }
    return font;
  }

  Font* loadTrueTypeFont(const char* filename, int height) override {
    return loadFreeTypeFont(filename, height);
  }
};

} // namespace she
