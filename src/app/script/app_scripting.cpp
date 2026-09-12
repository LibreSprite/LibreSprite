// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "delta/Extension.hpp"
#include "delta/Interpreter.hpp"
#include "delta/Shared.hpp"
#include "app/app.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "base/injection.h"
#include "base/path.h"
#include "base/range.h"
#include "base/string.h"
#include "base/trim_string.h"
#include "di.hpp"
#include "script/engine_delegate.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_type.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace {

std::unordered_set<std::string> loadedScripts;
std::shared_ptr<Interpreter> engine;
std::vector<std::shared_ptr<Extension>> extensions;
JSON::Value LS;

// The file of the most recently loaded script ("" if none).
std::string activeScript;

// REPL result echo (set by printLastResult()).
bool printLastResultFlag = false;

// The registered event handler (setEventHandler). Undefined until called.
JSON::Value eventHandler;

struct PendingEvent {
  std::string fileName;
  JSON::Value args;
};
Shared<std::deque<PendingEvent>> pendingEvents;

// Callbacks flushed after eval() returns (palette version bumps, deferred
// dialog builds, ...).
std::deque<std::function<void()>> afterEvalQueue;

class ScriptTimer : public ui::Timer {
public:
  ScriptTimer() : ui::Timer(1) {}
  void onTick() override {
    app::AppScripting::tick();
  }
};
std::unique_ptr<ScriptTimer> tickTimer;

// Routes text to the console delegate (DevConsole panel, --shell stdout)
// with a plain stdout fallback when no delegate is registered.
void routeToConsole(const std::string& text) {
  if (script::EngineDelegate::getRegistry().count("")) {
    inject<script::EngineDelegate> delegate;
    if (delegate) {
      delegate->onConsolePrint(text.c_str());
      return;
    }
  }
  std::cout << text << std::endl;
}

std::string formatReplValue(JSON::Value& value) {
  if (value.isString())
    return value.string();
  if (value.isNumber()) {
    double d = value.number();
    if (std::floor(d) == d && std::fabs(d) < 9007199254740992.0)
      return std::to_string(static_cast<long long>(d));
    std::ostringstream os;
    os << d;
    return os.str();
  }
  return value.toString();
}

} // anonymous namespace

namespace app {
  std::multimap<std::string, std::string> eventHooks;

  class EventListener : public ui::Widget {
  public:
    bool onProcessMessage(ui::Message* msg) override {
      auto event = ui::to_string(msg->type());
      auto range = base::range{eventHooks.equal_range(event)};
      if (range.empty())
        return false;

      JSON::Value args;
      args.push_back(event);
      args.push_back(static_cast<double>(msg->modifiers()));

      switch (msg->type()) {
      case ui::kKeyDownMessage:
      case ui::kKeyUpMessage:
        args.push_back(static_cast<double>(static_cast<ui::KeyMessage*>(msg)->unicodeChar()));
        args.push_back(static_cast<double>(static_cast<ui::KeyMessage*>(msg)->scancode()));
        break;

      case ui::kMouseDownMessage:
      case ui::kMouseUpMessage:
      case ui::kDoubleClickMessage:
      case ui::kMouseEnterMessage:
      case ui::kMouseLeaveMessage:
      case ui::kMouseMoveMessage:
      case ui::kSetCursorMessage:
      case ui::kMouseWheelMessage:
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->position().x));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->position().y));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->buttons()));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().x));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().y));
        break;

      default:
        break;
      }

      for (auto& entry : range)
        AppScripting::raiseEvent(entry.second, args);

      return false;
    }
  };

  std::unique_ptr<EventListener> listener;

  void AppScripting::clearEventHooks() {
    eventHooks.clear();
    if (listener) {
      ui::Manager::getDefault()->removeMessageListener(listener.get());
      listener.reset();
    }
  }

  void AppScripting::shutdown() {
    // Stop the tick pump + drop the message listener before the ui::Manager
    // is torn down.
    tickTimer.reset();
    clearEventHooks();
    // Release JS references held in app-side state before the interpreter
    // (and its runtime) goes away.
    eventHandler = JSON::Value{JSON::Special::Undefined};
    pendingEvents << [](auto& q){ q.clear(); };
    for (auto& ext : extensions)
      ext->shutdown();
    extensions.clear();
    // Destroy the engine (and any script-owned dialogs/widgets it holds) so
    // they don't outlive the ui::Manager.
    engine.reset();
  }

  bool AppScripting::scanScript(const std::string& fullPath) {
    auto extension = base::string_to_lower(base::get_file_extension(fullPath));
    if (extension != "js")
      return false;

    std::ifstream file{fullPath};

    if (!file)
      return false;

    std::string line;
    while (std::getline(file, line)) {
      base::trim_string(line, line);
      auto parts = base::split(line, ' ');

      for (auto& part : parts)
        base::trim_string(part, part);

      if (parts.empty() || parts[0].size() < 3 || parts[0].substr(0, 3) != "///")
        break;

      base::trim_string(parts[0].substr(3), parts[0]);
      if (parts[0].empty()) {
        parts.erase(parts.begin());
        if (parts.empty())
          break;
      }

      auto key = base::string_to_lower(parts[0]);
      parts.erase(parts.begin());

      if (key == "listen") {
        if (!listener) {
          listener.reset(new EventListener());
          ui::Manager::getDefault()->addMessageListener(listener.get());
        }
        for (auto& event : parts) {
          eventHooks.insert({event, fullPath});
        }
      }
    }

    return true;
  }

  void AppScripting::initEngine() {
    if (engine)
      return;

    App::instance()->Exit.connect([]{
      // Drop the timer while the UI Manager is still alive.
      tickTimer.reset();
      // Release JS references held in app-side state before the
      // interpreter (and its runtime) goes away.
      eventHandler = JSON::Value{JSON::Special::Undefined};
      pendingEvents << [](auto& q){ q.clear(); };
      for (auto& ext : extensions)
        ext->shutdown();
      extensions.clear();
      engine.reset();
    });

    engine = di::inject<Interpreter>("js");

    // Module search paths for bare import specifiers: the user's scripts
    // dir first, then the bundled data/scripts (mirrors ResourceFinder's
    // user/data dir resolution).
    {
      ResourceFinder rf{false};
      rf.includeUserDir("scripts");
      rf.includeDataDir("scripts");
      while (rf.next())
        engine->addModuleSearchPath(rf.filename());
    }

    LS["version"] = VERSION;
    LS["package"] = PACKAGE;
    engine->addGlobalValue("LS", LS);

    // Core event API: a single handler for all engine events.
    // setEventHandler(fn, ifEmpty?): with ifEmpty, only registers fn when
    // no handler is set yet (used by the onEvent auto-inject epilogue so
    // an explicit setEventHandler in the script wins).
    engine->addAPIFunction("setEventHandler", [](JSON::Array& args) -> JSON::Value {
      if (args.empty() || !args[0].isFunction())
        return JSON::Value{JSON::Special::Undefined};
      bool ifEmpty = args.size() > 1 && args[1].boolean();
      if (ifEmpty && !eventHandler.isUndefined())
        return JSON::Value{JSON::Special::Undefined};
      eventHandler = args[0];
      return JSON::Value{JSON::Special::Undefined};
    });

    extensions = di::injectAll<Extension>();
    for (auto& ext : extensions)
      addExtension(*ext);
  }

  void AppScripting::addExtension(Extension& ext) {
    for (auto& [name, func] : ext.getFunctions())
      engine->addAPIFunction(name, func);
    for (auto& [className, clazz] : ext.getClasses()) {
      auto& cls = engine->addClass(className, clazz->getBaseType(), clazz->getDerivedType(), clazz->getConstructor());
      for (auto& [name, method] : clazz->getMethods()) {
        cls.addMethod(name, method);
      }
      for (auto& [name, getset] : clazz->getGetSet()) {
        cls.addGetSet(name, getset.get, getset.set);
      }
    }
    auto boot = ext.init("js", LS);
    if (!boot.empty()) {
      try {
        engine->eval(boot, typeid(ext).name());
      } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to run boot script for extension " << typeid(ext).name() << ": " << e.what() << std::endl;
      }
    }
  }

  void AppScripting::drainEventQueue() {
    for (;;) {
      PendingEvent ev;
      bool got = false;
      pendingEvents << [&](auto& q){
        if (!q.empty()) {
          ev = std::move(q.front());
          q.pop_front();
          got = true;
        }
      };
      if (!got)
        break;

      // Lazily load the script that owns this event (first-event load for
      // ///listen scripts). Re-queue the event so the "init" event fired
      // by evalFile is delivered before it (legacy load-then-fire order).
      if (!ev.fileName.empty() && loadedScripts.find(ev.fileName) == loadedScripts.end()) {
        if (evalFile(ev.fileName)) {
          pendingEvents << [&](auto& q){ q.push_back(std::move(ev)); };
          continue;
        }
        continue; // file could not be loaded; drop the event
      }

      if (!eventHandler.isUndefined()) {
        JSON::Value ret = eventHandler(ev.args.array());
        // The delta JS->C++ wrapper converts a thrown JS exception into
        // an object with an "exception" key; surface it on the console.
        if (ret.isObject() && ret.object().contains("exception"))
          routeToConsole(ret["exception"].string());
      }
    }
  }

  void AppScripting::flushAfterEval() {
    while (!afterEvalQueue.empty()) {
      auto fn = std::move(afterEvalQueue.front());
      afterEvalQueue.pop_front();
      fn();
    }
  }

  void AppScripting::raiseEvent(const std::string& fileName, script::Value& event) {
    initEngine();
    if (!engine)
      return;
    pendingEvents << [&](auto& q){ q.emplace_back(fileName, event); };
  }

  bool AppScripting::eval(const std::string& code, const std::string& path) {
    initEngine();
    if (!engine)
      return false;
    loadedScripts.insert(path);
    try {
      // REPL lines eval as scripts: top-level `var` persists across lines
      // (matching the legacy REPL), and the last expression's completion
      // value is returned.
      JSON::Value result = engine->eval(code, path, Interpreter::EvalType::Script);
      if (printLastResultFlag && !result.isUndefined())
        routeToConsole(formatReplValue(result));
      flushAfterEval();
      return true;
    } catch (const std::exception& e) {
      routeToConsole(e.what());
      return false;
    }
  }

  bool AppScripting::evalFile(const std::string& fileName) {
    initEngine();
    if (!engine)
      return false;
    std::cout << "Reading file " << fileName << std::endl;
    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cout << "Could not open " << fileName << std::endl;
      return false;
    }
    auto source = std::string{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

    // Backward-compat epilogue: a legacy `function onEvent` becomes the
    // event handler unless the script already called setEventHandler.
    // Appended (not prepended) so `let`/`const` declarations in the script
    // are not shadowed, and `typeof onEvent` is safe at this point.
    source += "\nif (typeof onEvent === \"function\") setEventHandler(onEvent, true);\n";

    // Absolutize the entry path: it is the base for the entry module's
    // relative imports.
    std::string absPath = fileName;
    if (!std::filesystem::path(fileName).is_absolute())
      absPath = (std::filesystem::current_path() / fileName).string();

    // Re-run semantics: the most recently run script owns the event
    // handler, so drop the previous one and let the epilogue / an
    // explicit setEventHandler in the script re-register it. (The
    // event-driven lazy load in drainEventQueue only calls evalFile for
    // files not in loadedScripts, so this never re-evaluates on events.)
    eventHandler = JSON::Value{JSON::Special::Undefined};

    bool ok = false;
    try {
      engine->eval(source, absPath, Interpreter::EvalType::Module);
      ok = true;
    } catch (const std::exception& e) {
      routeToConsole(e.what());
    }

    if (ok) {
      loadedScripts.insert(fileName);
      activeScript = fileName;
      JSON::Value initEvent;
      initEvent.push_back("init");
      pendingEvents << [&](auto& q){ q.emplace_back(fileName, initEvent); };
      // Deliver "init" immediately (legacy semantics fired it
      // synchronously during evalFile).
      drainEventQueue();
      flushAfterEval();
    }
    return ok;
  }

  void AppScripting::printLastResult() {
    printLastResultFlag = true;
  }

  void AppScripting::tick() {
    if (!engine)
      return;
    engine->tick();
    drainEventQueue();
    for (auto& ext : extensions)
      ext->tick();
  }

  void AppScripting::startTickPump() {
    if (tickTimer)
      return;
    if (!ui::Manager::getDefault())
      return;
    tickTimer.reset(new ScriptTimer());
    tickTimer->start();
  }

  std::string AppScripting::getFileName() {
    return activeScript;
  }

  void AppScripting::afterEval(std::function<void()> fn) {
    afterEvalQueue.emplace_back(std::move(fn));
  }

  void AppScripting::consolePrint(const std::string& text) {
    routeToConsole(text);
  }

}
