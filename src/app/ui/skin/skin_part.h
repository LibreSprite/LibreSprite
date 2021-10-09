// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "gfx/size.h"

#include <memory>
#include <vector>

namespace she {
  class Surface;
}

namespace app {
  namespace skin {

    class SkinPart {
    public:
      typedef std::vector<she::Surface*> Bitmaps;

      SkinPart();
      ~SkinPart();

      std::size_t countBitmaps() const { return m_bitmaps.size(); }
      void clear();

      // It doesn't destroy the previous bitmap in the given "index".
      void setBitmap(std::size_t index, she::Surface* bitmap);

      she::Surface* bitmap(std::size_t index) const {
        return (index < m_bitmaps.size() ? m_bitmaps[index]: NULL);
      }

      she::Surface* bitmapNW() const { return bitmap(0); }
      she::Surface* bitmapN()  const { return bitmap(1); }
      she::Surface* bitmapNE() const { return bitmap(2); }
      she::Surface* bitmapE()  const { return bitmap(3); }
      she::Surface* bitmapSE() const { return bitmap(4); }
      she::Surface* bitmapS()  const { return bitmap(5); }
      she::Surface* bitmapSW() const { return bitmap(6); }
      she::Surface* bitmapW()  const { return bitmap(7); }

      gfx::Size size() const;

    private:
      Bitmaps m_bitmaps;
    };

    typedef std::shared_ptr<SkinPart> SkinPartPtr;

  } // namespace skin
} // namespace app
