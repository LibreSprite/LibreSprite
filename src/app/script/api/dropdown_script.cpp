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

class DropdownExtension : public Extension {
public:
  DropdownExtension() {
    auto& cls = addClass<void, ComboBoxObject>("Dropdown");
    // The dropdown is created by DialogObject::addDropDown() (C++), not
    // `new Dropdown()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<ComboBoxObject> {
      return std::make_shared<ComboBoxObject>();
    };

    addWidgetId<ComboBoxObject>(cls);

    // addOption(text) -> append a dropdown option.
    cls.addMethod("addOption") = [](ComboBoxObject& self, const std::string& text) -> JSON::Value {
      if (self.comboBox())
        self.comboBox()->addItem(text);
      return JSON::Value{JSON::Special::Undefined};
    };

    // count: the number of options.
    cls.addGetter("count") = [](ComboBoxObject& self) -> JSON::Value {
      return self.comboBox() ? (double)self.comboBox()->getItemCount() : (double)0;
    };

    // selected: the selected option's index (-1 if none). Setting it persists
    // to storage but does not raise the change event (setSelectedIndexSilent);
    // user selection does.
    cls.addGetter("selected") = [](ComboBoxObject& self) -> JSON::Value {
      return self.comboBox() ? (double)self.comboBox()->getSelectedItemIndex() : (double)-1;
    };
    cls.addSetter("selected") = [](ComboBoxObject& self, JSON::Value& v) {
      if (self.comboBox())
        self.comboBox()->setSelectedIndexSilent((int)v.number());
    };

    // value: the selected option's text ("" if none). Setting it selects the
    // matching option (setValueSilent); no change event is raised.
    cls.addGetter("value") = [](ComboBoxObject& self) -> JSON::Value {
      return self.comboBox() ? std::string{self.comboBox()->getValue()} : std::string{};
    };
    cls.addSetter("value") = [](ComboBoxObject& self, JSON::Value& v) {
      if (self.comboBox())
        self.comboBox()->setValueSilent(v.toString());
    };
  }
};

static di::provide<Extension, DropdownExtension> dropdownExt{"dropdown"};
