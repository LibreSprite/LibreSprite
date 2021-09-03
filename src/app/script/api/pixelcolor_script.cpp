// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "doc/color.h"
#include "script/engine.h"

class PixelColorScriptObject : public script::ScriptObject {
public:
    PixelColorScriptObject() {
        addFunction("rgba", doc::rgba).setDefault(0, 0, 0, 255);
        addFunction("rgbaR", doc::rgba_getr);
        addFunction("rgbaG", doc::rgba_getg);
        addFunction("rgbaB", doc::rgba_getb);
        addFunction("rgbaA", doc::rgba_geta);
        addFunction("graya", doc::graya).setDefault(0, 255);
        addFunction("grayaV", doc::graya_getv);
        addFunction("grayaA", doc::graya_geta);
    }
};

static script::ScriptObject::Regular<PixelColorScriptObject> reg("pixelColor");
