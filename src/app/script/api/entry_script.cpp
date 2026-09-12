// LibreSprite
// Copyright (C) 2023-2026  LibreSprite contributors
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

class EntryExtension : public Extension {
public:
  EntryExtension() {
    auto& cls = addClass<void, EntryObject>("Entry");
    // The entry is created by DialogObject::addEntry() (C++), not `new Entry()`
    // in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<EntryObject> {
      return std::make_shared<EntryObject>();
    };

    addWidgetId<EntryObject>(cls);

    // maxsize: the maximum number of characters.
    cls.addGetter("maxsize") = [](EntryObject& self) -> JSON::Value {
      return self.entry() ? (double)self.entry()->maxTextSize() : (double)0;
    };
    cls.addSetter("maxsize") = [](EntryObject& self, JSON::Value& v) {
      if (self.entry())
        self.entry()->setMaxTextSize((std::size_t)v.number());
    };

    // value: the entry text. Setting it does not raise the change event
    // (setTextSilent); user edits do.
    cls.addGetter("value") = [](EntryObject& self) -> JSON::Value {
      return self.entry() ? std::string{self.entry()->text()} : std::string{};
    };
    cls.addSetter("value") = [](EntryObject& self, JSON::Value& v) {
      if (self.entry())
        self.entry()->setTextSilent(v.toString());
    };
  }
};

static di::provide<Extension, EntryExtension> entryExt{"entry"};
