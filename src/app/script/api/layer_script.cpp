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

#include "doc/cel.h"
#include "doc/layer.h"

#include <memory>

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
  }
};

static di::provide<Extension, LayerExtension> x{"layer"};
