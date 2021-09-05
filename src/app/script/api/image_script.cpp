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
    addProperty("width", [this]{return m_image->width();});
    addProperty("height", [this]{return m_image->height();});
    addProperty("format", [this]{return (int) m_image->pixelFormat();});
    addFunction("getPixel", [this](int x, int y){return m_image->getPixel(x, y);});
    addMethod("putPixel", &ImageScriptObject::putPixel);
    addMethod("clear", &ImageScriptObject::clear);
  }

  void putPixel(int x, int y, int color) {
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
