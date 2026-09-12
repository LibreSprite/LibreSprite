// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/widget_script.h"

#include "she/system.h"
#include "she/surface.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

class ImageViewExtension : public Extension {
public:
  ImageViewExtension() {
    auto& cls = addClass<void, ImageViewObject>("ImageView");
    // The image view is created by DialogObject::addImageView() (C++), not
    // `new ImageView()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<ImageViewObject> {
      return std::make_shared<ImageViewObject>();
    };

    addWidgetId<ImageViewObject>(cls);

    // putImageData(data, width, height) -> copy RGBA pixel data into the view.
    // data is a Uint8Array (JSON ByteArray) of width*height*4 bytes.
    cls.addMethod("putImageData") = [](ImageViewObject& self, JSON::Value& data, double width, double height) -> JSON::Value {
      int w = (int)width;
      int h = (int)height;
      auto* view = self.imageView();
      if (!view)
        return JSON::Value{JSON::Special::Null};
      auto& bytes = data.byteArray();
      if (bytes.size() != (size_t)w * h * 4) {
        std::cout << "Error: data size " << bytes.size() << " does not match " << w << " * " << h << " * 4 (" << (w * h * 4) << ")." << std::endl;
        return JSON::Value{JSON::Special::Null};
      }
      auto* surface = view->getSurface();
      if (!surface || surface->width() != w || surface->height() != h) {
        surface = she::instance()->createRgbaSurface(w, h);
        view->setSurface(surface, true);
      }
      surface->lock();
      auto src = bytes.data();
      for (int y = 0; y < h; ++y) {
        std::copy(src, src + w * 4, surface->getData(0, y));
        src += w * 4;
      }
      surface->unlock();
      view->invalidate();
      return JSON::Value{JSON::Special::Undefined};
    };
  }
};

static di::provide<Extension, ImageViewExtension> imageViewExt{"imageview"};
