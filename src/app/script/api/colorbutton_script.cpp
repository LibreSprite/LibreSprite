// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/widget_script.h"

#include <memory>
#include <string>

class ColorExtension : public Extension {
public:
  ColorExtension() {
    auto& cls = addClass<void, ColorButtonObject>("Color");
    // The color button is created by DialogObject::addColor() (C++), not
    // `new Color()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<ColorButtonObject> {
      return std::make_shared<ColorButtonObject>();
    };

    addWidgetId<ColorButtonObject>(cls);

    // color: a 32-bit RGBA int (the pixelColor.rgba() format). Setting it
    // does not raise the change event (setColorSilent); user edits do.
    cls.addGetter("color") = [](ColorButtonObject& self) -> JSON::Value {
      if (!self.colorButton())
        return JSON::Value{(double)0};
      const app::Color c = self.colorButton()->getColor();
      return (double)doc::rgba((uint8_t)c.getRed(), (uint8_t)c.getGreen(),
                               (uint8_t)c.getBlue(), (uint8_t)c.getAlpha());
    };
    cls.addSetter("color") = [](ColorButtonObject& self, JSON::Value& v) {
      if (self.colorButton()) {
        uint32_t c = (uint32_t)v.number();
        self.colorButton()->setColorSilent(
          app::Color::fromRgb(doc::rgba_getr(c), doc::rgba_getg(c),
                              doc::rgba_getb(c), doc::rgba_geta(c)));
      }
    };
  }
};

static di::provide<Extension, ColorExtension> colorExt{"colorbutton"};
