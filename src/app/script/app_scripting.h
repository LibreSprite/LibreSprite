// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <functional>
#include <string>

#include "base/injection.h"
#include "script/value.h"

class Extension;

namespace app {

  class AppScripting {
    static void initEngine();
    static void addExtension(Extension& ext);
    static void drainEventQueue();
    static void flushAfterEval();
  public:
    static bool evalFile(const std::string& fileName);
    static void raiseEvent(const std::string& fileName, script::Value& event);
    static bool scanScript(const std::string& fullPath);
    static void clearEventHooks();

    // Evaluates a REPL line (script-type eval, so top-level `var` persists
    // across lines). Prints the completion value when printLastResult()
    // has been called.
    static bool eval(const std::string& code, const std::string& path = "::");
    static void printLastResult();

    // Pumps the event loop (timers, promises, microtasks) and
    // delivers queued events to the registered handler. Called once per
    // UI message-loop iteration by the tick pump.
    static void tick();
    // Starts the per-frame tick pump. Must be called once the UI message
    // loop is up (from App::run() in GUI mode).
    static void startTickPump();

    // The file of the most recently loaded script ("" if none).
    static std::string getFileName();
    // Queues a callback to run right after the next eval() returns.
    static void afterEval(std::function<void()> fn);

    // Routes text to the script console (DevConsole panel / --shell stdout,
    // plain stdout when no delegate is registered). Used by the timer
    // extension to surface errors thrown by timer callbacks.
    static void consolePrint(const std::string& text);

    static void shutdown();
  };

} // namespace app
