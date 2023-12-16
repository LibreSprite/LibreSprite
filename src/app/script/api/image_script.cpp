// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/base64.h"
#include "script/script_object.h"
#include "doc/image.h"
#include "she/surface.h"
#include "she/system.h"
#include <cstring>

class ImageScriptObject : public script::ScriptObject {
public:
  ImageScriptObject() {
    addProperty("width", [this]{return m_image->width();})
      .doc("read-only. The width of the image.");

    addProperty("height", [this]{return m_image->height();})
      .doc("read-only. The height of the image.");

    addProperty("stride", [this]{return m_image->getRowStrideSize();})
        .doc("read-only. The number of bytes per image row.");

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
      .docArg("color", "a 32-bit color in 8888 RGBA format.");

    addMethod("clear", &ImageScriptObject::clear)
      .doc("clears the image with the specified color.")
      .docArg("color", "a 32-bit color in 8888 RGBA format.");

    addMethod("putImageData", &ImageScriptObject::putImageData)
      .doc("writes the given pixels onto the image. Must be the same size as the image.")
      .docArg("data", "All of the pixels in the image.");

    addMethod("getImageData", &ImageScriptObject::getImageData)
      .doc("creates an array containing all of the image's pixels.")
      .docReturns("All pixels in an array (Lua) or a Uint8Array (JS)");

    addMethod("getPNGData", &ImageScriptObject::getPNGData)
      .doc("Encodes the image as a PNG.")
      .docReturns("The image as a Base64-encoded PNG string.");
  }

  void putImageData(script::Value::Buffer& data) {
    if (data.size() != std::size_t(m_image->getRowStrideSize()*m_image->height())) {
      std::cout << "Data size mismatch: " << data.size() << std::endl;
      return;
    }
    std::memcpy(m_image->getPixelAddress(0, 0), data.data(), data.size());
  }

  script::Value getImageData() {
    return {
      m_image->getPixelAddress(0, 0),
      std::size_t(m_image->getRowStrideSize()*m_image->height()),
      false
    };
  }

  std::string getPNGData() {
    auto w = m_image->width();
    auto h = m_image->height();
    std::shared_ptr<she::Surface> surface{
      she::instance()->createRgbaSurface(w, h),
      [](auto s) {s->dispose();}
    };
    if (!surface)
      return "";

    for (auto y = 0; y < h; ++y) {
      for (auto x = 0; x < w; ++x) {
        surface->putPixel(m_image->getPixel(x, y), x, y);
      }
    }

    std::string encoded;
    base::encode_base64(she::instance()->encodeSurfaceAsPNG(surface.get()), encoded);
    return "data:image/png;base64," + encoded;
  }

  void putPixel(int x, int y, int color) {
    if (unsigned(x) < unsigned(m_image->width()) && unsigned(y) < unsigned(m_image->height()))
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
