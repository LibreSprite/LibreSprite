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

class CheckBoxExtension : public Extension {
public:
  CheckBoxExtension() {
    auto& cls = addClass<void, CheckBoxObject>("CheckBox");
    // The check box is created by DialogObject::addCheckBox() (C++), not
    // `new CheckBox()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<CheckBoxObject> {
      return std::make_shared<CheckBoxObject>();
    };

    addWidgetId<CheckBoxObject>(cls);

    cls.addGetter("text") = [](CheckBoxObject& self) -> JSON::Value {
      return self.checkBox() ? std::string{self.checkBox()->text()} : std::string{};
    };
    cls.addSetter("text") = [](CheckBoxObject& self, JSON::Value& v) {
      if (self.checkBox())
        self.checkBox()->setText(v.toString());
    };

    // checked: whether the check box is checked (Widget::isSelected). Setting
    // it persists to storage but does not raise the change event
    // (setCheckedSilent); user clicks do.
    cls.addGetter("checked") = [](CheckBoxObject& self) -> JSON::Value {
      return self.checkBox() ? self.checkBox()->isSelected() : false;
    };
    cls.addSetter("checked") = [](CheckBoxObject& self, JSON::Value& v) {
      if (self.checkBox())
        self.checkBox()->setCheckedSilent(v.boolean());
    };
  }
};

static di::provide<Extension, CheckBoxExtension> checkBoxExt{"checkbox"};
