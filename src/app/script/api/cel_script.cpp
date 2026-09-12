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
#include "doc/image.h"

#include <memory>

class CelExtension : public Extension {
public:
  CelExtension() {
    auto& clazz = addClass<void, doc::Cel>("Cel");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Cel cannot be constructed directly"};
    };

    clazz.addGetter("x") = [](doc::Cel& cel) -> JSON::Value {
      return (double)cel.x();
    };
    clazz.addSetter("x") = [](doc::Cel& cel, JSON::Value& v) {
      cel.setPosition(static_cast<int>(v), cel.y());
    };

    clazz.addGetter("y") = [](doc::Cel& cel) -> JSON::Value {
      return (double)cel.y();
    };
    clazz.addSetter("y") = [](doc::Cel& cel, JSON::Value& v) {
      cel.setPosition(cel.x(), static_cast<int>(v));
    };

    clazz.addGetter("image") = [](doc::Cel& cel) -> JSON::Value {
      return JSON::makeNative(script_api::wrap(cel.image()));
    };

    clazz.addGetter("frame") = [](doc::Cel& cel) -> JSON::Value {
      return (double)cel.frame();
    };

    clazz.addMethod("setPosition") = [](doc::Cel& cel, double x, double y) -> JSON::Value {
      cel.setPosition((int)x, (int)y);
      return {};
    };
  }
};

static di::provide<Extension, CelExtension> x{"cel"};
