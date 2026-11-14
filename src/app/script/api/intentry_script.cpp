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

class IntEntryExtension : public Extension {
public:
  IntEntryExtension() {
    auto& cls = addClass<void, IntEntryObject>("IntEntry");
    // The int entry is created by DialogObject::addIntEntry() (C++), not
    // `new IntEntry()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<IntEntryObject> {
      return std::make_shared<IntEntryObject>();
    };

    addWidgetId<IntEntryObject>(cls);

    // min: the minimum value.
    cls.addGetter("min") = [](IntEntryObject& self) -> JSON::Value {
      return self.intEntry() ? (double)self.intEntry()->min() : (double)0;
    };
    cls.addSetter("min") = [](IntEntryObject& self, JSON::Value& v) {
      if (self.intEntry())
        self.intEntry()->setMin((int)v.number());
    };

    // max: the maximum value.
    cls.addGetter("max") = [](IntEntryObject& self) -> JSON::Value {
      return self.intEntry() ? (double)self.intEntry()->max() : (double)100;
    };
    cls.addSetter("max") = [](IntEntryObject& self, JSON::Value& v) {
      if (self.intEntry())
        self.intEntry()->setMax((int)v.number());
    };

    // value: the integer value. Setting it does not raise the change event
    // (setValueSilent); user edits do.
    cls.addGetter("value") = [](IntEntryObject& self) -> JSON::Value {
      return self.intEntry() ? (double)self.intEntry()->getValue() : (double)0;
    };
    cls.addSetter("value") = [](IntEntryObject& self, JSON::Value& v) {
      if (self.intEntry())
        self.intEntry()->setValueSilent((int)v.number());
    };
  }
};

static di::provide<Extension, IntEntryExtension> intEntryExt{"intentry"};
