// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "doc/color.h"
#include "doc/pixel_format.h"
#include "script/engine.h"

class ColorModeScriptObject : public script::ScriptObject {
public:
  ColorModeScriptObject() {
    addProperty("RGB", []{return doc::IMAGE_RGB;});
    addProperty("GRAYSCALE", []{return doc::IMAGE_GRAYSCALE;});
    addProperty("INDEXED", []{return doc::IMAGE_INDEXED;});
    addProperty("BITMAP", []{return doc::IMAGE_BITMAP;});
    makeGlobal("ColorMode");
  }
};

static script::ScriptObject::Regular<ColorModeScriptObject> reg("ColorModeScriptObject", {"global"});
