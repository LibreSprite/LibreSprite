// LibreSprite
// Copyright (C) 2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/cmd/add_frame_tag.h"
#include "app/cmd/set_frame_tag_anidir.h"
#include "app/cmd/set_frame_tag_color.h"
#include "app/cmd/set_frame_tag_name.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "doc/anidir.h"
#include "doc/frame_tag.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace {

doc::AniDir aniDirFromValue(JSON::Value& value) {
  if (value.isString()) {
    const auto name = static_cast<std::string>(value);
    if (name == "forward")
      return doc::AniDir::FORWARD;
    if (name == "reverse")
      return doc::AniDir::REVERSE;
    if (name == "pingpong")
      return doc::AniDir::PING_PONG;
    throw std::runtime_error{"Unknown frame tag animation direction: " + name};
  }

  const int valueNumber = static_cast<int>(value);
  if (valueNumber < 0 || valueNumber > 2)
    throw std::runtime_error{"Frame tag animation direction must be forward, reverse, pingpong, or 0..2"};
  return static_cast<doc::AniDir>(valueNumber);
}

doc::color_t colorFromValue(JSON::Value& value) {
  return static_cast<doc::color_t>(static_cast<uint64_t>(static_cast<double>(value)));
}

} // namespace

class FrameTagExtension : public Extension {
public:
  FrameTagExtension() {
    auto& clazz = addClass<void, doc::FrameTag>("FrameTag");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"FrameTag cannot be constructed directly"};
    };

    clazz.addGetter("fromFrame") = [](doc::FrameTag& tag) -> JSON::Value {
      return (double)tag.fromFrame();
    };
    clazz.addSetter("fromFrame") = [](doc::FrameTag& tag, JSON::Value& value) {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagRange(
        &tag, static_cast<int>(value), tag.toFrame()));
      tx.commit();
    };

    clazz.addGetter("toFrame") = [](doc::FrameTag& tag) -> JSON::Value {
      return (double)tag.toFrame();
    };
    clazz.addSetter("toFrame") = [](doc::FrameTag& tag, JSON::Value& value) {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagRange(
        &tag, tag.fromFrame(), static_cast<int>(value)));
      tx.commit();
    };

    clazz.addMethod("setFrameRange") = [](doc::FrameTag& tag, double from, double to) -> JSON::Value {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagRange(
        &tag, static_cast<int>(from), static_cast<int>(to)));
      tx.commit();
      return {};
    };

    clazz.addGetter("name") = [](doc::FrameTag& tag) -> JSON::Value {
      return tag.name();
    };
    clazz.addSetter("name") = [](doc::FrameTag& tag, JSON::Value& value) {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagName(&tag, static_cast<std::string>(value)));
      tx.commit();
    };

    clazz.addGetter("color") = [](doc::FrameTag& tag) -> JSON::Value {
      return (double)tag.color();
    };
    clazz.addSetter("color") = [](doc::FrameTag& tag, JSON::Value& value) {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagColor(&tag, colorFromValue(value)));
      tx.commit();
    };

    clazz.addGetter("aniDir") = [](doc::FrameTag& tag) -> JSON::Value {
      return doc::convert_to_string(tag.aniDir());
    };
    clazz.addSetter("aniDir") = [](doc::FrameTag& tag, JSON::Value& value) {
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetFrameTagAniDir(&tag, aniDirFromValue(value)));
      tx.commit();
    };
  }
};

static di::provide<Extension, FrameTagExtension> x{"frame_tag"};
