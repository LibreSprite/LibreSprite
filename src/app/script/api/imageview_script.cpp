// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "she/surface.h"
#include "she/system.h"
#include "ui/image_view.h"
#include "app/script/api/widget_script.h"

class ImageViewWidgetScriptObject : public WidgetScriptObject {
public:
  ImageViewWidgetScriptObject() {
      addMethod("setImageData", &ImageViewWidgetScriptObject::setImageData)
          .docArg("data", "Image pixel data")
          .docArg("width", "Width of the image in pixels")
          .docArg("height", "Height of the image in pixels");
  }

  void setImageData(script::Value::Buffer& data, int width, int height) {
      auto view = this->view();
      auto surface = view->getSurface();
      if (!surface || surface->width() != width || surface->height() != height) {
          surface = she::instance()->createRgbaSurface(width, height);
          view->setSurface(surface, true);
      }
      surface->lock();
      std::copy(data.data(), data.data() + data.size(), surface->getData(0, 0));
      surface->unlock();
      view->invalidate();
  }

  ui::ImageView* view() {
    auto view = getWidget<ui::ImageView>();
    if (!view)
      throw script::ObjectDestroyedException{};
    return view;
  }

  DisplayType getDisplayType() override {return DisplayType::Block;}
  Handle build() override {return new ui::ImageView();}
};

static script::ScriptObject::Regular<ImageViewWidgetScriptObject> _SO("ImageviewWidgetScriptObject", {
    "widget" + std::to_string(ui::kImageViewWidget)
  });
