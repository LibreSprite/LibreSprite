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

#include <memory>
#include <string>

class ButtonExtension : public Extension {
public:
  ButtonExtension() {
    auto& cls = addClass<void, ButtonObject>("Button");
    // The button is created by DialogObject::addButton() (C++), not `new
    // Button()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<ButtonObject> {
      return std::make_shared<ButtonObject>();
    };

    addWidgetId<ButtonObject>(cls);

    cls.addGetter("text") = [](ButtonObject& self) -> JSON::Value {
      return self.button() ? std::string{self.button()->text()} : std::string{};
    };
    cls.addSetter("text") = [](ButtonObject& self, JSON::Value& v) {
      if (self.button())
        self.button()->setText(v.toString());
    };
  }
};

static di::provide<Extension, ButtonExtension> buttonExt{"button"};
