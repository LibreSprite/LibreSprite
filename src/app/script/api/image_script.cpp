// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/script_object.h"
#include "doc/image.h"

class ImageScriptObject : public script::ScriptObject {
public:
  ImageScriptObject() {
    addProperty("width", [this]{return m_image->width();})
      .doc("read-only. The width of the image.");

    addProperty("height", [this]{return m_image->height();})
      .doc("read-only. The height of the image.");

    addProperty("format", [this]{return (int) m_image->pixelFormat();})
      .doc("read-only. The PixelFormat of the image.");

    addFunction("getPixel", [this](int x, int y){return m_image->getPixel(x, y);})
      .doc("reads a color from the given coordinate of the image.")
      .docArg("x", "integer")
      .docArg("y", "integer")
      .docReturns("a color value");

    addMethod("putPixel", &ImageScriptObject::putPixel)
      .doc("writes the color onto the image at the the given coordinate.")
      .docArg("x", "integer")
      .docArg("y", "integer")
      .docArg("color", "a 32-bit color in 8888 RGBA format");

    addMethod("clear", &ImageScriptObject::clear)
      .doc("clears the image with the specified color.")
      .docArg("color", "a 32-bit color in 8888 RGBA format");
  }

  void putPixel(int x, int y, int color) {
    if (unsigned(x) < m_image->width() && unsigned(y) < m_image->height())
      m_image->putPixel(x, y, color);
  }

  void clear(int color) {
    m_image->clear(color);
  }

  void* getWrapped() override {return m_image;}
  void setWrapped(void* image) override {
    m_image = static_cast<doc::Image*>(image);
  }

  Provides p{this, "activeImage"};
  doc::Image* m_image;
};

static script::ScriptObject::Regular<ImageScriptObject> imageSO("ImageScriptObject");
