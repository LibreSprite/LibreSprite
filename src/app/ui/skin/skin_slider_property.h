// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/skin/skin_property.h"
#include "gfx/rect.h"

#include <memory>

namespace ui {
  class Slider;
  class Graphics;
}

namespace app {
  namespace skin {

    class ISliderBgPainter {
    public:
      virtual ~ISliderBgPainter() { }
      virtual void paint(ui::Slider* slider, ui::Graphics* graphics, const gfx::Rect& rc) = 0;
    };

    class SkinSliderProperty : public ui::Property {
    public:
      static const char* Name;

      // The given painter is deleted automatically when this
      // property the destroyed.
      SkinSliderProperty(ISliderBgPainter* painter);
      ~SkinSliderProperty();

      ISliderBgPainter* getBgPainter() const;

    private:
      ISliderBgPainter* m_painter;
    };

    typedef std::shared_ptr<SkinSliderProperty> SkinSliderPropertyPtr;

  } // namespace skin
} // namespace app
