// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "doc/pixel_format.h"

#include <string>

// `ColorMode` is a plain constant object (no methods, no state), so it is
// injected as a frozen global from the extension's boot script rather than
// registered as a delta class.
class ColorModeExtension : public Extension {
public:
  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return "globalThis.ColorMode = Object.freeze({"
           "RGB: " + std::to_string(doc::IMAGE_RGB) + ", "
           "GRAYSCALE: " + std::to_string(doc::IMAGE_GRAYSCALE) + ", "
           "INDEXED: " + std::to_string(doc::IMAGE_INDEXED) + ", "
           "BITMAP: " + std::to_string(doc::IMAGE_BITMAP) + "});";
  }
};

static di::provide<Extension, ColorModeExtension> x{"colormode"};
