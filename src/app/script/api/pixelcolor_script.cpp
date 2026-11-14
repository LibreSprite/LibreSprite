// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "doc/color.h"
#include "app/script/api/script_api_common.h"

#include <memory>

class PixelColorExtension : public Extension {
public:
  PixelColorExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, PixelColorSite>("PixelColor");
    clazz.setConstructor() = []() -> std::shared_ptr<PixelColorSite> {
      static std::shared_ptr<PixelColorSite> site = std::make_shared<PixelColorSite>();
      return site;
    };

    clazz.addMethod("rgba") = [](PixelColorSite&, double r, double g, double b, JSON::Value& a) -> JSON::Value {
      uint8_t alpha = a.isUndefined() ? 0xFF : (uint8_t)static_cast<int>(a);
      return (double)doc::rgba((uint8_t)static_cast<int>(r),
                               (uint8_t)static_cast<int>(g),
                               (uint8_t)static_cast<int>(b), alpha);
    };

    clazz.addMethod("rgbaR") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::rgba_getr((uint32_t)c);
    };
    clazz.addMethod("rgbaG") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::rgba_getg((uint32_t)c);
    };
    clazz.addMethod("rgbaB") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::rgba_getb((uint32_t)c);
    };
    clazz.addMethod("rgbaA") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::rgba_geta((uint32_t)c);
    };

    clazz.addMethod("graya") = [](PixelColorSite&, double v, JSON::Value& a) -> JSON::Value {
      uint8_t alpha = a.isUndefined() ? 0xFF : (uint8_t)static_cast<int>(a);
      return (double)doc::graya((uint8_t)static_cast<int>(v), alpha);
    };
    clazz.addMethod("grayaV") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::graya_getv((uint16_t)c);
    };
    clazz.addMethod("grayaA") = [](PixelColorSite&, double c) -> JSON::Value {
      return (double)doc::graya_geta((uint16_t)c);
    };
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return "globalThis.pixelColor = new PixelColor();";
  }
};

static di::provide<Extension, PixelColorExtension> x{"pixelcolor"};
