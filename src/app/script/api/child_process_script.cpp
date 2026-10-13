// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// child_process extension: Node-style process spawning for scripts.
// spawn() starts a detached process; exec() runs one and captures its
// merged stdout+stderr. Blocking work never runs on the main thread: the
// pipe read runs on a TaskManager worker, and exit codes arrive from a
// reaper thread that also keeps detached children from becoming zombies.

#if !defined(__EMSCRIPTEN__) && !defined(ANDROID)

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/script/app_scripting.h"
#include "app/script/script_permissions.h"
#include "app/task_manager.h"
#include "base/launcher.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

// Headless liveness: the reaper thread is invisible to
// TaskManager::hasWork(), so keep the process alive while an exec promise
// is still pending (runHeadlessPump exits on the first idle round).
static std::atomic<int> pendingExecs{0};

namespace {

// Same default as Node's exec maxBuffer: a chatty child must not be able
// to grow the captured output without bound.
const size_t kMaxOutput = 1024 * 1024;

// Per-process state shared by the JS Process object, the reaper thread and
// the exec output reader. The atomics are written by other threads; the
// rest is main-thread only (the reader consumer and the reaper's delayed
// task both run there).
struct ProcessState {
  int pid = -1;
  void* processHandle = nullptr; // Windows: kept for the reaper

  std::atomic<bool> exited{false};
  std::atomic<int> exitCode{-1};
  std::atomic<bool> outputReady{false};

  // exec only
  std::string output;
  std::string error;
  std::shared_ptr<JSON::Function> callback;
  bool resolved = false; // main thread only
};

// Resolves a pending exec once both the output and the exit code are in.
// Main thread only; idempotent (the reader consumer and the reaper's
// delayed task may both reach it).
void resolveState(std::shared_ptr<ProcessState> state) {
  if (state->resolved || !state->exited || !state->outputReady)
    return;
  state->resolved = true;
  auto cb = std::move(state->callback);
  state->callback = nullptr;
  if (!cb)
    return; // abandoned by shutdown()
  pendingExecs--;

  JSON::Value rsp;
  rsp.object();
  if (!state->error.empty()) {
    rsp["error"] = state->error;
  } else {
    rsp["exitCode"] = static_cast<double>(state->exitCode);
    rsp["output"] = state->output;
  }
  JSON::Array args{std::move(rsp)};
  (*cb)(args);
}

struct ExecOutput {
  std::string text;
  std::string error;
};

} // namespace

#if defined(_WIN32)

namespace {

// Runs on a TaskManager worker thread: reads the capture pipe to EOF. On
// overflow the child is killed and an error is reported.
ExecOutput readOutputWin(int pid, void* pipeHandle) {
  ExecOutput out;
  char buffer[8192];
  DWORD n;
  while (ReadFile((HANDLE)pipeHandle, buffer, sizeof(buffer), &n, NULL) && n > 0) {
    out.text.append(buffer, n);
    if (out.text.size() > kMaxOutput)
      break;
  }
  CloseHandle((HANDLE)pipeHandle);
  if (out.text.size() > kMaxOutput) {
    base::launcher::terminate_process(pid);
    out.error = "Output exceeds 1 MiB; the process was terminated";
  }
  return out;
}

} // namespace

#else

namespace {

// Runs on a TaskManager worker thread: reads the capture pipe to EOF. On
// overflow the child is killed and an error is reported.
ExecOutput readOutputPosix(int pid, int pipeFd) {
  ExecOutput out;
  char buffer[8192];
  ssize_t n;
  while ((n = read(pipeFd, buffer, sizeof(buffer))) > 0) {
    out.text.append(buffer, n);
    if (out.text.size() > kMaxOutput)
      break;
  }
  close(pipeFd);
  if (out.text.size() > kMaxOutput) {
    base::launcher::terminate_process(pid);
    out.error = "Output exceeds 1 MiB; the process was terminated";
  }
  return out;
}

} // namespace

#endif

class Process {
public:
  explicit Process(std::shared_ptr<ProcessState> state) : state_{std::move(state)} {}

  int pid() const { return state_->pid; }
  // pid -1 (a never-started process): a no-op, since kill(-1) would signal
  // the whole process group on POSIX.
  bool kill() { return state_->pid > 0 && base::launcher::terminate_process(state_->pid); }
  bool exited() const { return state_->exited; }
  int exitCode() const { return state_->exitCode; }

private:
  std::shared_ptr<ProcessState> state_;
};

class ChildProcessExtension : public Extension {
public:
  ChildProcessExtension() {
    auto& cls = addClass<void, Process>("Process");
    // The Process is returned by child_process.spawn() (C++), not created
    // by `new Process()` in JS, but delta requires a non-null constructor.
    // This yields an invalid process (pid -1) and is never used by scripts.
    cls.setConstructor() = []() -> std::shared_ptr<Process> {
      return std::make_shared<Process>(std::make_shared<ProcessState>());
    };

    cls.addGetter("pid") = [](Process& p) -> JSON::Value {
      return JSON::Value{static_cast<double>(p.pid())};
    };
    cls.addMethod("kill") = [](Process& p) -> JSON::Value {
      return JSON::Value{p.kill()};
    };
    // null until the process exits.
    cls.addGetter("exitCode") = [](Process& p) -> JSON::Value {
      if (!p.exited())
        return JSON::Value{JSON::Special::Null};
      return JSON::Value{static_cast<double>(p.exitCode())};
    };

    // __cpSpawn(path, args) -> a Process object.
    // Throws if the permission is missing or the process cannot start.
    addFunction("__cpSpawn") = [this](const std::string& path, JSON::Value& args) -> JSON::Value {
      checkPermission();
      auto info = base::launcher::spawn_process(path, parseArgs(args), scriptCwd(), false);
      if (info.pid < 0)
        throw std::runtime_error("child_process.spawn: " + path + ": " + info.error);
      auto state = std::make_shared<ProcessState>();
      state->pid = info.pid;
      state->processHandle = info.processHandle;
      registerState(state);
      startReaperIfNeeded();
      return JSON::makeNative(std::make_shared<Process>(state));
    };

    // __cpExec(path, args, callback) -> undefined.
    // The callback receives {exitCode, output} or {error}.
    addFunction("__cpExec") = [this](const std::string& path, JSON::Value& args, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kAppLaunch)) {
        reject(cb, app::ScriptPermissions::denyMessage(app::ScriptPermissions::kAppLaunch));
        return JSON::Value{JSON::Special::Undefined};
      }
      std::vector<std::string> argv;
      try {
        argv = parseArgs(args);
      } catch (const std::exception& e) {
        reject(cb, e.what());
        return JSON::Value{JSON::Special::Undefined};
      }
      auto info = base::launcher::spawn_process(path, argv, scriptCwd(), true);
      if (info.pid < 0) {
        reject(cb, "child_process.exec: " + path + ": " + info.error);
        return JSON::Value{JSON::Special::Undefined};
      }

      auto state = std::make_shared<ProcessState>();
      state->pid = info.pid;
      state->processHandle = info.processHandle;
      state->callback = cb;
      trackCallback(cb);
      registerState(state);
      startReaperIfNeeded();
      pendingExecs++;

      // The read runs on a worker thread; the exit code arrives from the
      // reaper. The promise resolves when both are in (resolveState).
      app::TaskManager::instance().addTask<ExecOutput>(
        [pid = info.pid, fd = info.outputPipeFd, handle = info.outputPipe]() -> ExecOutput {
#if defined(_WIN32)
          return readOutputWin(pid, handle);
#else
          return readOutputPosix(pid, fd);
#endif
        },
        [state](ExecOutput&& out) {
          state->output = std::move(out.text);
          state->error = std::move(out.error);
          state->outputReady = true;
          resolveState(state);
        },
        [pid = info.pid]() {
          // App exit: kill the child so the blocking read unblocks.
          base::launcher::terminate_process(pid);
        });
      return JSON::Value{JSON::Special::Undefined};
    };
  }

  bool keepAlive() override {
    return pendingExecs > 0;
  }

  void shutdown() override {
    // Stop the reaper before releasing the JS callbacks it may deliver.
    stopReaper();
    // Release JS references on the main thread, before the engine goes
    // away; the tasks that still reference them only hold the C++ state.
    {
      std::lock_guard<std::mutex> guard(registryMutex_);
      for (auto& [pid, state] : registry_)
        state->callback = nullptr;
      registry_.clear();
    }
    {
      std::lock_guard<std::mutex> guard(callbackMutex_);
      pendingCallbacks_.clear();
    }
    pendingExecs = 0;
  }

  std::string init(const std::string& language, JSON::Value&) override {
    if (language != "js")
      return "";
    return R"(
globalThis.child_process = {};

// Starts a detached process (no shell, no wait): it runs independently of
// the script and outlives the script run. The process inherits the
// environment and runs with the script's directory as working directory.
// Throws if it cannot be started.
child_process.spawn = function(path, args) {
    return __cpSpawn(path, args || []);
};

// Runs a process and resolves with {exitCode, output} when it exits
// (output is the merged stdout+stderr) without blocking the app.
// Rejects if the process cannot be started or its output exceeds 1 MiB.
child_process.exec = function(path, args) {
    return new Promise((ok, nok) => {
        __cpExec(path, args || [], (response) => {
            if (response && response.error) {
                nok(new Error(response.error));
            } else {
                ok({exitCode: response.exitCode, output: response.output});
            }
        });
    });
};
)";
  }

private:
  static void checkPermission() {
    if (!app::ScriptPermissions::check(app::ScriptPermissions::kAppLaunch))
      throw std::runtime_error(app::ScriptPermissions::denyMessage(app::ScriptPermissions::kAppLaunch));
  }

  static std::vector<std::string> parseArgs(JSON::Value& v) {
    if (!v.isArray())
      throw std::runtime_error("child_process: args must be an array of strings");
    std::vector<std::string> args;
    for (auto& a : v.array()) {
      if (!a.isString())
        throw std::runtime_error("child_process: args must be an array of strings");
      args.push_back(a.string());
    }
    return args;
  }

  // The child's working directory: the running script's directory, or the
  // app's cwd when there is no file context (REPL, headless eval).
  static std::string scriptCwd() {
    std::string script = app::AppScripting::getFileName();
    if (script.empty())
      return "";
    auto parent = std::filesystem::path(script).parent_path();
    return parent.empty() ? "" : parent.string();
  }

  static void reject(const std::shared_ptr<JSON::Function>& cb, const std::string& message) {
    JSON::Value rsp;
    rsp.object();
    rsp["error"] = message;
    JSON::Array args{std::move(rsp)};
    (*cb)(args);
  }

  // JS callbacks of in-flight execs, released on the main thread (see
  // shutdown()): the tasks referencing them run on worker threads and must
  // not destroy the QuickJS values.
  void trackCallback(const std::shared_ptr<JSON::Function>& cb) {
    std::lock_guard<std::mutex> guard(callbackMutex_);
    pendingCallbacks_.push_back(cb);
  }

  void registerState(std::shared_ptr<ProcessState> state) {
    std::lock_guard<std::mutex> guard(registryMutex_);
    registry_[state->pid] = state;
  }

  void startReaperIfNeeded() {
    bool expected = false;
    if (!reaperRunning_.compare_exchange_strong(expected, true))
      return;
    reaper_ = std::thread([this] { reaperLoop(); });
  }

  void stopReaper() {
    if (!reaperRunning_.exchange(false))
      return;
    if (reaper_.joinable())
      reaper_.join();
  }

  // Collects the exit codes of spawned children (without it, detached
  // children stay zombies until the app exits). Only the pids registered
  // here are reaped, so the launcher's own waitpid() children
  // (app.launch / open_file) are left alone.
  void reaperLoop() {
    for (;;) {
      std::vector<std::pair<int, void*>> procs;
      {
        std::lock_guard<std::mutex> guard(registryMutex_);
        for (auto& [pid, state] : registry_)
          procs.emplace_back(pid, state->processHandle);
      }
      for (auto& [pid, handle] : procs) {
        auto code = base::launcher::try_reap(pid, handle);
        if (!code)
          continue;
        std::shared_ptr<ProcessState> state;
        {
          std::lock_guard<std::mutex> guard(registryMutex_);
          auto it = registry_.find(pid);
          if (it != registry_.end()) {
            state = it->second;
            registry_.erase(it);
          }
        }
        if (!state)
          continue;
        state->exitCode = *code;
        state->exited = true;
        // Deliver the resolution on the main thread; a plain spawn has no
        // callback and only needs the atomics set above.
        if (state->callback)
          app::TaskManager::instance().delayed([state] { resolveState(state); });
      }
      if (!reaperRunning_)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  std::mutex registryMutex_;
  std::map<int, std::shared_ptr<ProcessState>> registry_;
  std::thread reaper_;
  std::atomic<bool> reaperRunning_{false};

  std::mutex callbackMutex_;
  std::vector<std::shared_ptr<JSON::Function>> pendingCallbacks_;
};

static di::provide<Extension, ChildProcessExtension> cpExt{"child_process"};

#endif // !__EMSCRIPTEN__ && !ANDROID
