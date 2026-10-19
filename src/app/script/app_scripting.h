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

#include "script/value.h"

namespace app {
  class Document;
  class Transaction;

  class AppScripting {
  public:
    static bool evalFile(const std::string& fileName);
    static void raiseEvent(const std::string& fileName, script::Value& event);
    static bool scanScript(const std::string& fullPath);
    static void clearEventHooks();

    // Loads every script that declares "///listen app_boot" and delivers
    // it an "app_boot" event (each script's "init" event first),
    // draining the event queue synchronously so the boot handlers run
    // before App::initialize returns. Runs once per process, before CLI
    // files are opened.
    static void fireBootEvents();

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
    // Headless counterpart of startTickPump(): pumps tick() until the
    // script is idle (no pending events, no due extension work, no
    // keepAlive() extension), then returns so the process can exit.
    // Called from App::run() when no UI message loop runs.
    static void runHeadlessPump();

    // The file of the executing script (the script token on top of the
    // interpreter token stack), or the most recently loaded script when
    // no script token is active ("" if none).
    static std::string getFileName();

    // Critical-section counter (app.critical): while it is > 0 the env is
    // not torn down by a re-run. enterCritical() increments the running
    // script's counter and returns the file to pass to leaveCritical()
    // ("" when no script token is active: a no-op). Main thread only.
    static std::string enterCritical();
    static void leaveCritical(const std::string& file);
    // app.transaction() scope: enterTransaction() opens a scope for the
    // running script (or joins the outer one when nested) and returns
    // the file to pass to leaveTransaction() ("" when no script env is
    // active: a no-op). While a scope is open, the script's tools.*
    // calls join its transaction, so the group commits as one undo
    // step. A scope also holds the critical counter, so a re-run is
    // refused while it is open (the same env protection as
    // app.critical). Main thread only.
    static std::string enterTransaction();
    static void leaveTransaction(const std::string& file, bool commit);
    // The running script's active scope transaction (the tool driver
    // joins it as an external transaction), or nullptr when no scope
    // is open.
    static Transaction* activeTransaction();
    // The document the active scope is bound to (the document active
    // at enter time), or nullptr when no scope is open.
    static Document* activeTransactionDocument();
    // True when some script's open app.transaction() scope is bound to
    // the given document (destroying it would dangle the scope's
    // Transaction undo-history pointers). Scans every env: the caller
    // may run outside any script (a GUI close).
    static bool transactionScopeHolds(Document* doc);
    // Queues a callback to run right after the next eval() returns.
    static void afterEval(std::function<void()> fn);

    // Routes text to the script console (DevConsole panel / --shell stdout,
    // plain stdout when no delegate is registered). Used by the timer
    // extension to surface errors thrown by timer callbacks.
    static void consolePrint(const std::string& text);

    static void shutdown();
  };

} // namespace app
