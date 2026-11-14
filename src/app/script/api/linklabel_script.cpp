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

class LinkLabelExtension : public Extension {
public:
  LinkLabelExtension() {
    auto& cls = addClass<void, LinkLabelObject>("LinkLabel");
    // The link label is created by DialogObject::addLinkLabel() (C++), not
    // `new LinkLabel()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<LinkLabelObject> {
      return std::make_shared<LinkLabelObject>();
    };

    addWidgetId<LinkLabelObject>(cls);

    cls.addGetter("text") = [](LinkLabelObject& self) -> JSON::Value {
      auto* linkLabel = self.linkLabel();
      return linkLabel ? std::string{linkLabel->text()} : std::string{};
    };
    cls.addSetter("text") = [](LinkLabelObject& self, JSON::Value& v) {
      if (auto* linkLabel = self.linkLabel())
        linkLabel->setText(v.toString());
    };
    cls.addGetter("url") = [](LinkLabelObject& self) -> JSON::Value {
      auto* linkLabel = self.linkLabel();
      return linkLabel ? std::string{linkLabel->url()} : std::string{};
    };
    cls.addSetter("url") = [](LinkLabelObject& self, JSON::Value& v) {
      if (auto* linkLabel = self.linkLabel())
        linkLabel->setUrl(v.toString());
    };
  }
};

static di::provide<Extension, LinkLabelExtension> linkLabelExt{"linklabel"};
