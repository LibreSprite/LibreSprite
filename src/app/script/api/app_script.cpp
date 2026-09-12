// LibreSprite
// Copyright (C) 2015-2016  David Capello
// Copyright (C) 2023-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"
#include "app/script/api/widget_script.h"

#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "base/launcher.h"
#include "doc/document.h"
#include "doc/site.h"
#include "ui/manager.h"

#include <memory>
#include <string>

class AppObject {
public:
  static app::Document* activeDocument() {
    return app::UIContext::instance()->activeDocument();
  }
};

class AppExtension : public Extension {
public:
  AppExtension() {
    auto& clazz = addClass<void, AppObject>("App");
    clazz.setConstructor() = []() -> std::shared_ptr<AppObject> {
      return std::make_shared<AppObject>();
    };

    clazz.addGetter("activeFrameNumber") = [](AppObject&) -> JSON::Value {
      if (!app::current_editor)
        return (double)0;
      return (double)app::current_editor->getSite().frame();
    };

    clazz.addGetter("activeLayerNumber") = [](AppObject&) -> JSON::Value {
      if (!app::current_editor)
        return (double)0;
      return (double)static_cast<int>(app::current_editor->getSite().layerIndex());
    };

    clazz.addGetter("activeImage") = [](AppObject&) -> JSON::Value {
      if (!app::current_editor)
        return JSON::Value{JSON::Special::Null};
      auto* img = app::current_editor->getSite().image();
      if (!img)
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(script_api::wrap<doc::Image>(img));
    };

    // The `Sprite` proxy re-resolves the active document, so a fresh
    // SpriteSite behaves identically to the `sprite` global.
    clazz.addGetter("activeSprite") = [](AppObject&) -> JSON::Value {
      if (!AppObject::activeDocument())
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(std::make_shared<script_api::SpriteSite>());
    };

    clazz.addGetter("activeDocument") = [](AppObject&) -> JSON::Value {
      auto* doc = AppObject::activeDocument();
      if (!doc)
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(script_api::wrap<doc::Document>(doc));
    };

    clazz.addGetter("version") = [](AppObject&) -> JSON::Value {
      return std::string{VERSION};
    };

    clazz.addGetter("platform") = [](AppObject&) -> JSON::Value {
      #ifdef EMSCRIPTEN
      return std::string{"emscripten"};
      #elif defined(_WIN32)
      return std::string{"windows"};
      #elif defined(__APPLE__)
      return std::string{"macos"};
      #elif defined(ANDROID)
      return std::string{"android"};
      #else
      return std::string{"linux"};
      #endif
    };

    clazz.addMethod("open") = [](AppObject&, const std::string& fn) -> JSON::Value {
      if (fn.empty())
        return JSON::Value{JSON::Special::Null};
      auto* ctx = app::UIContext::instance();
      auto* oldDoc = ctx->activeDocument();
      auto* openCmd = app::CommandsModule::instance()->getCommandByName(app::CommandId::OpenFile);
      app::Params params;
      params.set("filename", fn.c_str());
      ctx->executeCommand(openCmd, params);
      auto* newDoc = ctx->activeDocument();
      if (newDoc == oldDoc)
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(script_api::wrap<doc::Document>(newDoc));
    };

    clazz.addMethod("launch") = [](AppObject&, const std::string& cmd) -> JSON::Value {
      return base::launcher::open_file(cmd);
    };

    clazz.addMethod("redraw") = [](AppObject&) -> JSON::Value {
      ui::Manager::getDefault()->invalidate();
      return JSON::Value{JSON::Special::Undefined};
    };

    // createDialog() -> a new Dialog object. The dialog is built (grid layout
    // + openWindow) after the current eval finishes (see DialogObject).
    // Requires the GUI (ui::Manager); in a headless --shell there is no UI
    // system, so creating the ui::Dialog would crash (null widget list) —
    // return null instead.
    clazz.addMethod("createDialog") = [](AppObject&) -> JSON::Value {
      if (!ui::Manager::getDefault())
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(std::make_shared<DialogObject>());
    };
  }

  std::string init(const std::string&, JSON::Value&) override {
    return R"(
      globalThis.app = new App();
      // Lazily expose the pixelColor / command globals as sub-objects (the
      // same instances as the top-level globals). Resolved on access so the
      // order in which the other extensions' boot scripts run doesn't matter.
      Object.defineProperty(app, "pixelColor", {
        configurable: true, get: function() { return globalThis.pixelColor; }
      });
      Object.defineProperty(app, "command", {
        configurable: true, get: function() { return globalThis.command; }
      });
      // Yield to the event loop: a Promise resolved on the next tick (the
      // timer extension fires it). Scripts use `await app.yield()`.
      app.yield = function() {
        return new Promise(function(resolve) { setTimeout(resolve, 0); });
      };
    )";
  }
};

static di::provide<Extension, AppExtension> appExt{"app"};
