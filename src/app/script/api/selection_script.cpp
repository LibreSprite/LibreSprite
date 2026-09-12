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

#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/document.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "gfx/rect.h"

#include <memory>
#include <stdexcept>

namespace {
  app::Document* activeDocument() {
    auto* doc = app::UIContext::instance()->activeDocument();
    if (!doc)
      throw std::runtime_error{"No active document"};
    return doc;
  }
} // namespace

class SelectionExtension : public Extension {
public:
  SelectionExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, SelectionSite>("Selection");
    clazz.setConstructor() = []() -> std::shared_ptr<SelectionSite> {
      static std::shared_ptr<SelectionSite> site = std::make_shared<SelectionSite>();
      return site;
    };

    clazz.addMethod("select") = [](SelectionSite&, double x, double y, double w, double h) -> JSON::Value {
      auto* doc = activeDocument();
      doc::Mask newMask;
      if (w > 0 && h > 0)
        newMask.replace(gfx::Rect((int)x, (int)y, (int)w, (int)h));
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetMask(doc, &newMask));
      tx.commit();
      return {};
    };

    clazz.addMethod("selectAll") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      doc::Mask newMask;
      newMask.replace(doc->sprite()->bounds());
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetMask(doc, &newMask));
      tx.commit();
      return {};
    };

    clazz.addMethod("deselect") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::DeselectMask(doc));
      tx.commit();
      return {};
    };

    clazz.addGetter("bounds") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      if (!doc->isMaskVisible())
        return JSON::Value{JSON::Special::Null};
      gfx::Rect b = doc->mask()->bounds();
      return JSON::makeObject({
        {"x", (double)b.x},
        {"y", (double)b.y},
        {"width", (double)b.w},
        {"height", (double)b.h},
      });
    };
  }
};

static di::provide<Extension, SelectionExtension> x{"selection"};
