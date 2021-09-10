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
      .doc("Converts R, G, B, A values into a single 32-bit RGBA color.")
      .docArg("r", "red, 0-255.")
      .docArg("g", "green, 0-255.")
      .docArg("b", "blue, 0-255.")
      .docArg("a", "alpha (opacity), 0-255.")
      .docReturns("A 32-bit color in 8888 RGBA format.")
      .setDefault(0, 0, 0, 255);

    addFunction("rgbaR", doc::rgba_getr)
      .doc("Extracts the red channel from a 32-bit color")
      .docArg("color", "A 32-bit color in 8888 RGBA format")
      .docReturns("The red component of the color");

    addFunction("rgbaG", doc::rgba_getg)
      .doc("Extracts the green channel from a 32-bit color")
      .docArg("color", "A 32-bit color in 8888 RGBA format")
      .docReturns("The green component of the color");

    addFunction("rgbaB", doc::rgba_getb)
      .doc("Extracts the blue channel from a 32-bit color")
      .docArg("color", "A 32-bit color in 8888 RGBA format")
      .docReturns("The blue component of the color");

    addFunction("rgbaA", doc::rgba_geta)
      .doc("Extracts the alpha channel from a 32-bit color")
      .docArg("color", "A 32-bit color in 8888 RGBA format")
      .docReturns("The alpha component of the color");

    addFunction("graya", doc::graya)
      .docArg("gray", "The luminance of color")
      .docArg("alpha", "The alpha (opacity) of the color)")
      .docReturns("The color with the given luminance/opacity")
      .setDefault(0, 255);

    addFunction("grayaV", doc::graya_getv)
      .doc("Extracts the luminance from a 32-bit color")
      .docArg("color", "A 32-bit color in 888 RGBA format")
      .docReturns("The luminance Value of the color");

    addFunction("grayaA", doc::graya_geta)
      .doc("Extracts the alpha (opacity) from a 32-bit color")
      .docArg("color", "A 32-bit color in 888 RGBA format")
      .docReturns("The alpha component of the color");
  }
};

static script::ScriptObject::Regular<PixelColorScriptObject> reg("pixelColor");
