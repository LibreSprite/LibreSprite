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
    addFunction("rgba", doc::rgba)
      .arg("r", "Red, 0-255")
      .arg("g", "Green, 0-255")
      .arg("b", "Blue, 0-255")
      .arg("a", "Alpha (opacity), 0-255")
      .returns("A 32-bit color in 8888 RGBA format")
      .setDefault(0, 0, 0, 255);
    addFunction("rgbaR", doc::rgba_getr)
      .arg("color", "A 32-bit color in 8888 RGBA format")
      .returns("The red component of the color")
      .documentation("Extracts the red channel from a 32-bit color");
    addFunction("rgbaG", doc::rgba_getg)
      .arg("color", "A 32-bit color in 8888 RGBA format")
      .returns("The green component of the color")
      .documentation("Extracts the green channel from a 32-bit color");
    addFunction("rgbaB", doc::rgba_getb)
      .arg("color", "A 32-bit color in 8888 RGBA format")
      .returns("The blue component of the color")
      .documentation("Extracts the blue channel from a 32-bit color");
    addFunction("rgbaA", doc::rgba_geta)
      .arg("color", "A 32-bit color in 8888 RGBA format")
      .returns("The alpha component of the color")
      .documentation("Extracts the alpha channel from a 32-bit color");
    addFunction("graya", doc::graya)
      .arg("gray", "The luminance of color")
      .arg("alpha", "The alpha (opacity) of the color)")
      .returns("The color with the given luminance/opacity")
      .setDefault(0, 255);
    addFunction("grayaV", doc::graya_getv)
      .arg("color", "A 32-bit color in 888 RGBA format")
      .returns("The luminance Value of the color")
      .documentation("Extracts the luminance from a 32-bit color");
    addFunction("grayaA", doc::graya_geta)
      .arg("color", "A 32-bit color in 888 RGBA format")
      .returns("The alpha component of the color")
      .documentation("Extracts the alpha (opacity) from a 32-bit color");
  }
};

static script::ScriptObject::Regular<PixelColorScriptObject> reg("pixelColor");
