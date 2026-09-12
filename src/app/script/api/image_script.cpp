// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "base/base64.h"
#include "doc/image.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/manager.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

class ImageExtension : public Extension {
public:
  ImageExtension() {
    auto& clazz = addClass<void, doc::Image>("Image");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Image cannot be constructed directly"};
    };

    clazz.addGetter("width") = [](doc::Image& img) -> JSON::Value {
      return (double)img.width();
    };
    clazz.addGetter("height") = [](doc::Image& img) -> JSON::Value {
      return (double)img.height();
    };
    clazz.addGetter("stride") = [](doc::Image& img) -> JSON::Value {
      return (double)img.getRowStrideSize();
    };
    clazz.addGetter("format") = [](doc::Image& img) -> JSON::Value {
      return (double)img.pixelFormat();
    };

    clazz.addMethod("getPixel") = [](doc::Image& img, double x, double y) -> JSON::Value {
      return (double)img.getPixel((int)x, (int)y);
    };

    clazz.addMethod("putPixel") = [](doc::Image& img, double x, double y, double color) -> JSON::Value {
      if ((unsigned)x < (unsigned)img.width() && (unsigned)y < (unsigned)img.height())
        img.putPixel((int)x, (int)y, (doc::color_t)color);
      return {};
    };

    clazz.addMethod("clear") = [](doc::Image& img, double color) -> JSON::Value {
      img.clear((doc::color_t)color);
      return {};
    };

    clazz.addMethod("putImageData") = [](doc::Image& img, JSON::Value& data) -> JSON::Value {
      auto& bytes = data.byteArray();
      if (bytes.size() != std::size_t(img.getRowStrideSize() * img.height())) {
        std::cout << "Data size mismatch: " << bytes.size() << std::endl;
        return {};
      }
      std::memcpy(img.getPixelAddress(0, 0), bytes.data(), bytes.size());
      if (auto* mgr = ui::Manager::getDefault())
        mgr->invalidate();
      return {};
    };

    clazz.addMethod("getImageData") = [](doc::Image& img) -> JSON::Value {
      auto* addr = img.getPixelAddress(0, 0);
      std::size_t size = std::size_t(img.getRowStrideSize() * img.height());
      auto vec = std::make_shared<std::vector<uint8_t>>(addr, addr + size);
      return JSON::Value{vec};
    };

    clazz.addMethod("getPNGData") = [](doc::Image& img) -> JSON::Value {
      auto w = img.width();
      auto h = img.height();
      std::shared_ptr<she::Surface> surface{
        she::instance()->createRgbaSurface(w, h),
        [](she::Surface* s) { s->dispose(); }
      };
      if (!surface)
        return std::string{};

      for (auto y = 0; y < h; ++y)
        for (auto x = 0; x < w; ++x)
          surface->putPixel(img.getPixel(x, y), x, y);

      std::string encoded;
      base::encode_base64(she::instance()->encodeSurfaceAsPNG(surface.get()), encoded);
      return std::string{"data:image/png;base64,"} + encoded;
    };
  }
};

static di::provide<Extension, ImageExtension> x{"image"};
