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
#include <stdexcept>
#include <vector>

namespace {
  // The wrapper re-resolves the site's image on every call: a cel trim or a
  // canvas resize may replace (or remove) the image object underneath.
  doc::Image& resolveImage(script_api::ImageSite& site) {
    auto* img = site.image();
    if (!img)
      throw std::runtime_error{"No image at this site"};
    return *img;
  }
} // namespace

class ImageExtension : public Extension {
public:
  ImageExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, ImageSite>("Image");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Image cannot be constructed directly"};
    };

    clazz.addGetter("width") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).width();
    };
    clazz.addGetter("height") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).height();
    };
    clazz.addGetter("stride") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).getRowStrideSize();
    };
    clazz.addGetter("format") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).pixelFormat();
    };

    clazz.addMethod("getPixel") = [](ImageSite& site, double x, double y) -> JSON::Value {
      return (double)resolveImage(site).getPixel((int)x, (int)y);
    };

    clazz.addMethod("putPixel") = [](ImageSite& site, double x, double y, double color) -> JSON::Value {
      auto& img = resolveImage(site);
      if ((unsigned)x < (unsigned)img.width() && (unsigned)y < (unsigned)img.height())
        img.putPixel((int)x, (int)y, (doc::color_t)color);
      return {};
    };

    clazz.addMethod("clear") = [](ImageSite& site, double color) -> JSON::Value {
      resolveImage(site).clear((doc::color_t)color);
      return {};
    };

    clazz.addMethod("putImageData") = [](ImageSite& site, JSON::Value& data) -> JSON::Value {
      auto& img = resolveImage(site);
      auto& bytes = data.byteArray();
      if (bytes.size() != std::size_t(img.getRowStrideSize()) * img.height()) {
        std::cout << "Data size mismatch: " << bytes.size() << std::endl;
        return {};
      }
      std::memcpy(img.getPixelAddress(0, 0), bytes.data(), bytes.size());
      if (auto* mgr = ui::Manager::getDefault())
        mgr->invalidate();
      return {};
    };

    clazz.addMethod("getImageData") = [](ImageSite& site) -> JSON::Value {
      auto& img = resolveImage(site);
      auto* addr = img.getPixelAddress(0, 0);
      std::size_t size = std::size_t(img.getRowStrideSize()) * img.height();
      auto vec = std::make_shared<std::vector<uint8_t>>(addr, addr + size);
      return JSON::Value{vec};
    };

    clazz.addMethod("getPNGData") = [](ImageSite& site) -> JSON::Value {
      auto& img = resolveImage(site);
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

      auto png = she::instance()->encodeSurfaceAsPNG(surface.get());
      if (png.empty())
        throw std::runtime_error{"Failed to encode PNG data"};
      std::string encoded;
      base::encode_base64(png, encoded);
      return std::string{"data:image/png;base64,"} + encoded;
    };
  }
};

static di::provide<Extension, ImageExtension> x{"image"};
