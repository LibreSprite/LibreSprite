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

#include "app/document.h"
#include "app/document_api.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <memory>
#include <stdexcept>

// `Layer` wraps a `doc::Layer` (a snapshot of a specific layer, obtained via
// `sprite.layer(i)`).
class LayerExtension : public Extension {
public:
  LayerExtension() {
    auto& clazz = addClass<void, doc::Layer>("Layer");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Layer cannot be constructed directly"};
    };

    clazz.addGetter("name") = [](doc::Layer& layer) -> JSON::Value {
      return std::string{layer.name()};
    };
    clazz.addSetter("name") = [](doc::Layer& layer, JSON::Value& v) {
      layer.setName(static_cast<std::string>(v));
    };

    clazz.addGetter("isImage") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isImage();
    };
    clazz.addGetter("isBackground") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isBackground();
    };
    clazz.addGetter("isTransparent") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isTransparent();
    };

    clazz.addGetter("isVisible") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isVisible();
    };
    clazz.addSetter("isVisible") = [](doc::Layer& layer, JSON::Value& v) {
      layer.setVisible(static_cast<bool>(v));
    };

    clazz.addGetter("isEditable") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isEditable();
    };
    clazz.addSetter("isEditable") = [](doc::Layer& layer, JSON::Value& v) {
      layer.setEditable(static_cast<bool>(v));
    };

    clazz.addGetter("isMovable") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isMovable();
    };
    clazz.addGetter("isContinuous") = [](doc::Layer& layer) -> JSON::Value {
      return layer.isContinuous();
    };
    clazz.addGetter("flags") = [](doc::Layer& layer) -> JSON::Value {
      return (double)static_cast<int>(layer.flags());
    };

    clazz.addGetter("celCount") = [](doc::Layer& layer) -> JSON::Value {
      if (layer.isImage())
        return (double)static_cast<doc::LayerImage*>(&layer)->getCelsCount();
      return 0.0;
    };

    clazz.addMethod("cel") = [](doc::Layer& layer, double i) -> JSON::Value {
      return JSON::makeNative(layer.cel((doc::frame_t)i));
    };

    // The ASSERT in DocumentApi::addCel is debug-only, so the wrapper checks
    // the preconditions itself and throws (surfaces as a JS exception).
    clazz.addMethod("createCel") = [](doc::Layer& layer, double i) -> JSON::Value {
      if (!layer.isImage())
        throw std::runtime_error{"createCel: not an image layer"};
      auto frame = static_cast<doc::frame_t>(i);
      auto* li = static_cast<doc::LayerImage*>(&layer);
      auto* sprite = li->sprite();
      if (frame < 0 || frame >= sprite->totalFrames())
        throw std::runtime_error{"createCel: frame out of range"};
      if (li->cel(frame) != nullptr)
        throw std::runtime_error{"createCel: a cel already exists at this frame"};

      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      auto* doc = static_cast<app::Document*>(sprite->document());
      doc::ImageRef image(doc::Image::create(sprite->pixelFormat(), sprite->width(), sprite->height()));
      auto cel = doc->getApi(tx).addCel(li, frame, image);
      tx.commit();
      return JSON::makeNative(script_api::wrap(cel.get()));
    };
  }
};

static di::provide<Extension, LayerExtension> x{"layer"};
