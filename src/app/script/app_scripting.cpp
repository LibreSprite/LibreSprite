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
#include "app/script/script_permission_dialog.h"
#include "app/script/script_permissions.h"
#include "app/task_manager.h"
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

#include <chrono>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// The scripts that ran at least once (canonical paths): drainEventQueue
// uses it to lazy-load the script that owns an event.
std::unordered_set<std::string> loadedScripts;
JSON::Value LS;

// One interpreter environment per script file: its own QuickJS runtime
// and context, its own extension set, and its own event handler. Scripts
// do not share globals; the REPL is the dedicated "::" env, running
// unrestricted.
struct ScriptEnv {
  std::shared_ptr<Interpreter> engine;
  std::vector<std::shared_ptr<Extension>> extensions;
  // The registered event handler (setEventHandler). Undefined until called.
  JSON::Value eventHandler;
  // The script context token (an interned canonical path, or the
  // unrestricted token for the REPL env): the interpreter pushes it
  // around every JS entry point.
  const void* token = nullptr;
};

// Canonical script path (the REPL env is "::") -> its environment.
// std::map: node addresses are stable, so the bootstrap lambdas can
// capture the env by reference.
std::map<std::string, ScriptEnv> envs;

// The file of the most recently loaded script ("" if none): the
// getFileName() fallback when no script token is active, and the target
// of events that carry no file name.
std::string activeScript;

// REPL result echo (set by printLastResult()).
bool printLastResultFlag = false;

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

// Canonical form of a script path (absolute + lexically normalized, the
// same form as AppScripting::activeScript). Used to match event file
// names against loadedScripts no matter how the path was passed in.
std::string canonicalScriptPath(const std::string& path) {
  std::filesystem::path p{path};
  if (!p.is_absolute())
    p = std::filesystem::current_path() / p;
  return p.lexically_normal().string();
}

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

// Tears down one env: releases the JS references held in extension and
// app-side state (ext->shutdown(), the event handler) before the
// interpreter (and its runtime) goes away, then drops the env.
void destroyEnv(std::map<std::string, ScriptEnv>::iterator it) {
  for (auto& ext : it->second.extensions)
    ext->shutdown();
  it->second.eventHandler = JSON::Value{JSON::Special::Undefined};
  it->second.extensions.clear();
  it->second.engine.reset();
  envs.erase(it);
}

// Tears down every env and the pending events (app exit).
void teardownEnvs() {
  pendingEvents << [](auto& q){ q.clear(); };
  while (!envs.empty())
    destroyEnv(envs.begin());
}

// Registers ext's functions and classes on env's interpreter and runs
// its boot script (if any).
void addExtension(ScriptEnv& env, Extension& ext) {
  for (auto& [name, func] : ext.getFunctions())
    env.engine->addAPIFunction(name, func);
  for (auto& [className, clazz] : ext.getClasses()) {
    auto& cls = env.engine->addClass(className, clazz->getBaseType(), clazz->getDerivedType(), clazz->getConstructor());
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
      env.engine->eval(boot, typeid(ext).name());
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to run boot script for extension " << typeid(ext).name() << ": " << e.what() << std::endl;
    }
  }
}

// Creates (on first use) and bootstraps the env for a canonical script
// path: its own interpreter (module search paths, the LS global, the
// setEventHandler API bound to this env's handler) and its own extension
// set (all the Extension provides are non-singleton).
ScriptEnv& getEnv(const std::string& path) {
  if (envs.empty())
    app::App::instance()->Exit.connect([]{
      // Drop the timer while the UI Manager is still alive.
      tickTimer.reset();
      teardownEnvs();
    });
  auto& env = envs.try_emplace(path, ScriptEnv{}).first->second;
  if (env.engine)
    return env;

  // The REPL env ("::") runs unrestricted; script envs run under the
  // script's permission token.
  env.token = (path == "::") ? app::ScriptPermissions::kUnrestrictedToken
                             : app::ScriptPermissions::tokenFor(path);
  env.engine = di::inject<Interpreter>("js");
  env.engine->setToken(env.token);

  // Module search paths for bare import specifiers: the user's scripts
  // dir first, then the bundled data/scripts (mirrors ResourceFinder's
  // user/data dir resolution).
  {
    app::ResourceFinder rf{false};
    rf.includeUserDir("scripts");
    rf.includeDataDir("scripts");
    while (rf.next())
      env.engine->addModuleSearchPath(rf.filename());
  }

  LS["version"] = VERSION;
  LS["package"] = PACKAGE;
  env.engine->addGlobalValue("LS", LS);

  // Core event API: a single handler for all engine events, bound to this
  // env (setEventHandler(fn, ifEmpty?): with ifEmpty, only registers fn
  // when no handler is set yet (used by the onEvent auto-inject epilogue
  // so an explicit setEventHandler in the script wins)).
  env.engine->addAPIFunction("setEventHandler", [&env](JSON::Array& args) -> JSON::Value {
    if (args.empty() || !args[0].isFunction())
      return JSON::Value{JSON::Special::Undefined};
    bool ifEmpty = args.size() > 1 && args[1].boolean();
    if (ifEmpty && !env.eventHandler.isUndefined())
      return JSON::Value{JSON::Special::Undefined};
    env.eventHandler = args[0];
    return JSON::Value{JSON::Special::Undefined};
  });

  env.extensions = di::injectAll<Extension>();
  for (auto& ext : env.extensions)
    addExtension(env, *ext);
  return env;
}

} // anonymous namespace

namespace app {
  // Prompts for the missing permissions of a script (or auto-grants them
  // when there is no UI); returns false to abort the run.
  static bool ensurePermissions(const std::string& path, const std::set<std::string>& requested);

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
    // Destroy the envs (and any script-owned dialogs/widgets they hold) so
    // they don't outlive the ui::Manager.
    teardownEnvs();
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

  // Delivers queued events to the handler of the env that owns the event's
  // file (an event with no file goes to the last-run env).
  static void drainEventQueue() {
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

      // Normalize the path the same way evalFile stores it so a
      // relative/absolute mismatch does not reload an already loaded
      // script. An event with no file goes to the last-run env.
      std::string normPath = ev.fileName.empty() ? std::string{} : canonicalScriptPath(ev.fileName);
      if (normPath.empty())
        normPath = activeScript;
      if (normPath.empty())
        continue; // no script has been run yet: drop the event

      // Lazily load the script that owns this event (first-event load for
      // ///listen scripts). Re-queue the event so the "init" event fired
      // by evalFile is delivered before it (legacy load-then-fire order).
      if (loadedScripts.find(normPath) == loadedScripts.end()) {
        if (AppScripting::evalFile(normPath)) {
          pendingEvents << [&](auto& q){ q.push_back(std::move(ev)); };
          continue;
        }
        continue; // file could not be loaded; drop the event
      }

      auto envIt = envs.find(normPath);
      if (envIt == envs.end())
        continue; // the env was torn down; drop the event
      ScriptEnv& env = envIt->second;
      if (!env.eventHandler.isUndefined()) {
        JSON::Value ret = env.eventHandler(ev.args.array());
        // The delta JS->C++ wrapper converts a thrown JS exception into
        // an object with an "exception" key; surface it on the console.
        if (ret.isObject() && ret.object().contains("exception"))
          routeToConsole(ret["exception"].string());
      }
    }
  }

  // Flushes the afterEval callbacks (palette version bumps, deferred
  // dialog builds, ...) queued during the last eval.
  static void flushAfterEval() {
    while (!afterEvalQueue.empty()) {
      auto fn = std::move(afterEvalQueue.front());
      afterEvalQueue.pop_front();
      fn();
    }
  }

  void AppScripting::raiseEvent(const std::string& fileName, script::Value& event) {
    // The event is routed to its owning env (or lazily loads the script)
    // when it is drained; no env is needed here.
    pendingEvents << [&](auto& q){ q.emplace_back(fileName, event); };
  }

  bool AppScripting::eval(const std::string& code, const std::string& path) {
    // The REPL env is dedicated (path "::", unrestricted token): REPL
    // lines are user-typed code, not gated by script permissions, and do
    // not share globals with scripts.
    ScriptEnv& env = getEnv(path);
    try {
      // REPL lines eval as scripts: top-level `var` persists across lines
      // (matching the legacy REPL), and the last expression's completion
      // value is returned.
      JSON::Value result = env.engine->eval(code, path, Interpreter::EvalType::Script);
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

    // Lexically normalized (".."/"." collapsed) rather than the raw
    // argument - an unnormalized relative invocation like
    // `--script ../foo.js` would otherwise make every default-domain
    // storage call fail for a script that did nothing wrong.
    auto canonicalPath = std::filesystem::path(absPath).lexically_normal().string();

    // Permission gate: the script requests API access via "///permission"
    // header lines; the first run prompts for the missing grants (or
    // auto-grants them when there is no UI to prompt in). A cancelled
    // prompt aborts the run.
    auto requested = ScriptPermissions::parseRequested(canonicalPath);
    if (!ensurePermissions(canonicalPath, requested))
      return false;

    // A re-run replaces the previous instance: its env (imported submodule
    // state, extension timers, dialogs, servers) is torn down, and a fresh
    // env re-executes the entry module and its imports. After the
    // permission gate, so a cancelled prompt keeps the running instance.
    // (The event-driven lazy load in drainEventQueue only calls evalFile
    // for files not in loadedScripts, so events never replace.)
    if (auto it = envs.find(canonicalPath); it != envs.end()) {
      // Refuse a re-run requested from inside the still-executing instance
      // (command.RunScript): tearing the env down would free the runtime
      // this call runs in.
      if (Interpreter::hasToken(it->second.token)) {
        routeToConsole("Script already running, re-run it later: " + canonicalPath);
        return false;
      }
      destroyEnv(it);
    }
    ScriptEnv& env = getEnv(canonicalPath);

    activeScript = canonicalPath;

    // The env's token is pushed by the interpreter around the eval, so
    // the script (and the continuations it creates, stamped with it at
    // marshal time) runs under its own permission grants.
    bool ok = false;
    try {
      env.engine->eval(source, absPath, Interpreter::EvalType::Module);
      ok = true;
    } catch (const std::exception& e) {
      routeToConsole(e.what());
    }

    if (ok) {
      // Store this call's canonical path (not activeScript, which a nested
      // evalFile during the eval - e.g. command.RunScript - may have
      // clobbered), so event-driven lookups do not re-load the script.
      loadedScripts.insert(canonicalPath);
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

  static bool ensurePermissions(const std::string& path, const std::set<std::string>& requested) {
    auto granted = ScriptPermissions::granted(path);
    std::set<std::string> missing;
    for (auto& perm : requested)
      if (!granted.count(perm))
        missing.insert(perm);
    if (missing.empty())
      return true;

    if (!App::instance()->isGui()) {
      // Headless (--batch/--shell): auto-grant so batch runs keep
      // working; the prompt is a GUI-only concern (it is a blocking
      // modal loop and would hang the run).
      for (auto& perm : missing)
        granted.insert(perm);
      ScriptPermissions::setGranted(path, granted);
      std::string list;
      for (auto& perm : missing)
        list += (list.empty() ? "" : " ") + perm;
      std::cout << "Warning: no UI to prompt for permissions; auto-granted "
                << list << " for " << path << std::endl;
      return true;
    }

    std::set<std::string> chosen;
    if (!ScriptPermissionDialog::show(path, missing, chosen))
      return false;
    for (auto& perm : chosen)
      granted.insert(perm);
    ScriptPermissions::setGranted(path, granted);
    return true;
  }

  void AppScripting::printLastResult() {
    printLastResultFlag = true;
  }

  void AppScripting::tick() {
    // The single-engine sequence, across every env: settle the engines
    // (promises, microtasks), deliver queued events, then fire due
    // extension work.
    for (auto& [path, env] : envs)
      env.engine->tick();
    drainEventQueue();
    for (auto& [path, env] : envs)
      for (auto& ext : env.extensions)
        ext->tick();
  }

  void AppScripting::startTickPump() {
    if (tickTimer)
      return;
    tickTimer.reset(new ScriptTimer());
    tickTimer->start();
  }

  void AppScripting::runHeadlessPump() {
    if (envs.empty())
      return;
    using namespace std::chrono;
    ui::Manager* mgr = ui::Manager::getDefault();
    for (;;) {
      // Pump the ui::Manager loop (MessageLoop::pumpMessages() minus the
      // display flip and the sleep): timers tick and listeners receive
      // messages, but nothing is blitted. generateMessages() skips the
      // timer poll when the manager has no child windows (headless), so
      // poll the timers here too, and pump the queue either way.
      if (mgr) {
        ui::Timer::pollTimers();
        mgr->generateMessages();
        mgr->pumpQueue();
        mgr->collectGarbage();
      }

      // Deliver worker-thread results (fetch, fs, server handlers) queued
      // by TaskManager; in GUI mode its 10ms ui::Timer does this.
      if (app::TaskManager::exists())
        app::TaskManager::instance().pump();

      // Same sequence as tick() (which the GUI runs per frame): pump the
      // engines (promises, microtasks), deliver queued events, then ask
      // each extension for its next wake time (0 = idle) while firing due
      // work.
      for (auto& [path, env] : envs)
        env.engine->tick();
      drainEventQueue();
      milliseconds nextWake{0};
      for (auto& [path, env] : envs) {
        for (auto& ext : env.extensions) {
          auto wake = ext->tick();
          if (wake > milliseconds(0) && (nextWake == milliseconds(0) || wake < nextWake))
            nextWake = wake;
        }
      }
      // Settle the work queued by the above (e.g. a fired timer resolving a
      // promise) before checking for idle.
      for (auto& [path, env] : envs)
        env.engine->tick();

      // The scripts are active while work is pending: events to deliver,
      // due extension work, tasks in TaskManager (fs, fetch, server
      // handlers), or a keepAlive() extension (e.g. a listening HTTP
      // server). The check spans all envs: one script's work keeps the
      // pump running for the others.
      bool active = nextWake > milliseconds(0);
      pendingEvents >> [&](auto& q){ if (!q.empty()) active = true; };
      if (app::TaskManager::exists() && app::TaskManager::instance().hasWork())
        active = true;
      if (!active) {
        for (auto& [path, env] : envs) {
          for (auto& ext : env.extensions) {
            if (ext->keepAlive()) {
              active = true;
              break;
            }
          }
          if (active)
            break;
        }
      }
      if (!active)
        break;

      // Sleep until the next wake, capped at the TaskManager poll period so
      // worker results are delivered with the same latency as in GUI mode.
      milliseconds sleepFor = nextWake > milliseconds(0) ? nextWake : milliseconds(10);
      if (sleepFor > milliseconds(10))
        sleepFor = milliseconds(10);
      std::this_thread::sleep_for(sleepFor);
    }
  }

  std::string AppScripting::getFileName() {
    // The executing script's token (an interned canonical path, the same
    // reverse lookup as ScriptPermissions::check()) when one is on top of
    // the interpreter token stack; the most recently loaded script
    // otherwise (C++ call sites, the REPL).
    const void* token = Interpreter::currentToken();
    if (token && token != ScriptPermissions::kUnrestrictedToken)
      return std::string{static_cast<const char*>(token)};
    return activeScript;
  }

  void AppScripting::afterEval(std::function<void()> fn) {
    afterEvalQueue.emplace_back(std::move(fn));
  }

  void AppScripting::consolePrint(const std::string& text) {
    routeToConsole(text);
  }

}
