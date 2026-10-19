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

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/script/app_scripting.h"
#include "app/script/script_permissions.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "base/launcher.h"
#include "doc/document.h"
#include "doc/site.h"
#include "she/she.h"
#include "ui/manager.h"

#include <fstream>
#include <memory>
#include <stdexcept>
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

    // The wrapper re-resolves its site's image on every call (see ImageSite):
    // a stroke's cel trim or a canvas resize may replace the image object.
    clazz.addGetter("activeImage") = [](AppObject&) -> JSON::Value {
      if (!app::current_editor)
        return JSON::Value{JSON::Special::Null};
      auto site = app::current_editor->getSite();
      if (!site.image())
        return JSON::Value{JSON::Special::Null};
      return JSON::makeNative(std::make_shared<script_api::ImageSite>(site));
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

    clazz.addGetter("isHeadless") = [](AppObject&) -> JSON::Value {
      return JSON::Value{app::App::instance()->isHeadless()};
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

    // A script-controlled target is an RCE risk; it is gated on the
    // per-script, user-approved app.launch permission (the same grant
    // covers child_process.spawn/exec).
    clazz.addMethod("launch") = [](AppObject&, const std::string& cmd) -> JSON::Value {
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kAppLaunch)) {
        app::AppScripting::consolePrint(app::ScriptPermissions::denyMessage(app::ScriptPermissions::kAppLaunch));
        return JSON::Value{false};
      }
      return base::launcher::open_file(cmd);
    };

    clazz.addMethod("redraw") = [](AppObject&) -> JSON::Value {
      ui::Manager::getDefault()->invalidate();
      return JSON::Value{JSON::Special::Undefined};
    };

    // takeScreenshot(filename) -> saves the whole UI (the display surface,
    // which also contains the dialogs) as a PNG. Returns false when there
    // is no display (no ui::Manager). Headless runs capture the virtual
    // surface of the dummy video driver.
    clazz.addMethod("takeScreenshot") = [](AppObject&, const std::string& fileName) -> JSON::Value {
      auto* mgr = ui::Manager::getDefault();
      if (!mgr || !mgr->getDisplay())
        return false;
      she::Surface* src = mgr->getDisplay()->getSurface();
      if (!src || src->width() <= 0 || src->height() <= 0)
        return false;

      auto surface = std::shared_ptr<she::Surface>(
        she::instance()->createRgbaSurface(src->width(), src->height()),
        [](she::Surface* s) { s->dispose(); });
      if (!surface)
        return false;

      src->blitTo(surface.get(), 0, 0, 0, 0, src->width(), src->height());

      auto png = she::instance()->encodeSurfaceAsPNG(surface.get());
      if (png.empty())
        throw std::runtime_error{"Failed to encode screenshot as PNG"};

      std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
      if (!file.good())
        throw std::runtime_error{"Unable to open file for writing: " + fileName};
      file.write(reinterpret_cast<const char*>(png.data()),
                 static_cast<std::streamsize>(png.size()));
      if (!file.good())
        throw std::runtime_error{"Failed while writing file: " + fileName};
      return true;
    };

    // createDialog() -> a new Dialog object. The dialog is built (grid layout
    // + openWindow) after the current eval finishes (see DialogObject).
    // Requires a ui::Manager: headless --batch/--shell runs create one on
    // the dummy video driver (the dialog is painted into the virtual
    // surface), so it returns null only when no manager exists at all.
    clazz.addMethod("createDialog") = [](AppObject&, const std::string& id) -> JSON::Value {
      if (!ui::Manager::getDefault())
        return JSON::Value{JSON::Special::Null};

      auto dobj = std::make_shared<DialogObject>();
      if (!id.empty())
          dobj->dialog()->setId(id.c_str());

      return JSON::makeNative(dobj);
    };

    // app.critical support (the JS glue is in the boot script below):
    // __criticalEnter() increments the calling script's env critical
    // counter and returns the env key to pass to __criticalLeave() (""
    // when no script token is active: a no-op). While the counter is > 0
    // a re-run of the script is refused (the env is not torn down), so an
    // async continuation of the script cannot outlive its env.
    addFunction("__criticalEnter") = []() -> JSON::Value {
      return app::AppScripting::enterCritical();
    };

    addFunction("__criticalLeave") = [](const std::string& file) -> JSON::Value {
      app::AppScripting::leaveCritical(file);
      return JSON::Value{JSON::Special::Undefined};
    };

    // app.transaction support (the JS glue is in the boot script below):
    // __transactionEnter() opens the calling script's transaction scope
    // (or joins the outer one when nested) and returns the env key to
    // pass to __transactionLeave() ("" when no script env is active: a
    // no-op). While the scope is open, the script's tools.* calls join
    // its transaction, so the group commits as one undo step. The scope
    // also holds the env's critical counter, so a re-run of the script
    // is refused while it is open (the env is not torn down), as with
    // app.critical.
    addFunction("__transactionEnter") = []() -> JSON::Value {
      return app::AppScripting::enterTransaction();
    };

    addFunction("__transactionLeave") = [](const std::string& file, bool commit) -> JSON::Value {
      app::AppScripting::leaveTransaction(file, commit);
      return JSON::Value{JSON::Special::Undefined};
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
      Object.defineProperty(app, "tools", {
        configurable: true, get: function() { return globalThis.tools; }
      });
      // Yield to the event loop: a Promise resolved on the next tick (the
      // timer extension fires it). Scripts use `await app.yield()`.
      app.yield = function() {
        return new Promise(function(resolve) { setTimeout(resolve, 0); });
      };
      // Run fn (sync or async) with the env protected from teardown: while
      // fn runs - including any async work it awaits - a re-run of the
      // script is refused and the env is not torn down, so an async
      // continuation cannot outlive its env. Returns fn's result (a
      // Promise when fn is async), so callers can `await app.critical(...)`.
      app.critical = function(fn) {
        if (typeof fn !== "function")
          throw new TypeError("app.critical: fn must be a function");
        const key = __criticalEnter();
        let result;
        try {
          result = fn();
        } catch (err) {
          __criticalLeave(key);
          throw err;
        }
        // Async fn: hold the counter across the async window, release it on
        // settle (resolve or reject - a rejecting fn must not leave the
        // counter set). The value / rejection is passed through.
        if (key && result && typeof result.then === "function")
          return result.then(
            v => { __criticalLeave(key); return v; },
            e => { __criticalLeave(key); throw e; }
          );
        // Sync fn: the counter is only held during the call.
        __criticalLeave(key);
        return result;
      };
      // Run fn (sync or async) with a shared transaction: all tools.*
      // calls inside fn join the transaction and commit as one undo
      // step when fn settles (a throwing fn rolls the whole group back,
      // the document is exactly as before the call). Returns fn's
      // result (a Promise when fn is async), so callers can
      // `await app.transaction(...)`.
      app.transaction = function(fn) {
        if (typeof fn !== "function")
          throw new TypeError("app.transaction: fn must be a function");
        const key = __transactionEnter();
        let result;
        try {
          result = fn();
        } catch (err) {
          __transactionLeave(key, false); // abort: roll back the group
          throw err;
        }
        // Async fn: hold the scope across the async window, settle it
        // on resolve (commit) or reject (rollback). The value /
        // rejection is passed through.
        if (key && result && typeof result.then === "function")
          return result.then(
            v => { __transactionLeave(key, true); return v; },
            e => { __transactionLeave(key, false); throw e; }
          );
        // Sync fn: commit on success.
        __transactionLeave(key, true);
        return result;
      };
    )";
  }
};

static di::provide<Extension, AppExtension> appExt{"app"};
