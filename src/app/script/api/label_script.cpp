// LibreSprite
// Copyright (C) 2023-2026 LibreSprite contributors
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

class LabelExtension : public Extension {
public:
  LabelExtension() {
    auto& cls = addClass<void, LabelObject>("Label");
    // Labels are created by dialog.addLabel() (C++), not `new Label()` in JS,
    // but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<LabelObject> {
      return std::make_shared<LabelObject>();
    };
    addWidgetId<LabelObject>(cls);
    cls.addGetter("text") = [](LabelObject& self) -> JSON::Value {
      auto* label = self.label();
      return label ? std::string{label->text()} : std::string{};
    };
    cls.addSetter("text") = [](LabelObject& self, JSON::Value& v) {
      if (auto* label = self.label())
        label->setText(v.toString());
    };
  }
};

static di::provide<Extension, LabelExtension> labelExt{"label"};
