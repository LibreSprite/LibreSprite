// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "delta/JSON.hpp"

#include <chrono>
#include <future>
#include <map>
#include <string>
#include <vector>

namespace app {

  // In-flight script codec requests (format_load / format_save events).
  //
  // ScriptFileFormat creates a request and raises the codec event
  // (worker thread in GUI, main thread in headless). The script's codec
  // replies through the "file" API (file.onLoad/onSave/onLoadError/
  // onSaveError) or the watchdog expires the request; both reply paths
  // run on the main thread, so a request is settled at most once.
  class ScriptCodecRequests {
  public:
    // Watchdog window: the loop-visible time the codec has between the
    // last reset and the reply. The sweep only runs while the main loop
    // pumps, so synchronous codec work - which blocks the loop - cannot
    // expire the window; it bounds the time between yields instead.
    static constexpr int kWatchdogMillis = 2000;

    struct Created {
      int id;
      std::future<JSON::Value> future;
    };

    // Creates the request and arms the watchdog. Any thread.
    static Created create() {
      Request req;
      req.deadline = std::chrono::steady_clock::now() +
                     std::chrono::milliseconds(kWatchdogMillis);
      auto future = req.promise.get_future();
      int id;
      {
        base::scoped_lock lock(impl().mutex);
        id = impl().nextId++;
        impl().requests.emplace(id, std::move(req));
      }
      return {id, std::move(future)};
    }

    // Settles the request with the codec's reply (main thread only).
    static bool resolve(int id, JSON::Value value) {
      return settle(id, std::move(value));
    }

    // Settles the request as a failure with a message (main thread only).
    static bool fail(int id, const std::string& message) {
      JSON::Value err;
      err.object()["error"] = message;
      return settle(id, std::move(err));
    }

    // Re-arms the watchdog (file.resetWatchdog). Main thread only.
    static bool reset(int id) {
      base::scoped_lock lock(impl().mutex);
      auto it = impl().requests.find(id);
      if (it == impl().requests.end() || it->second.resolved)
        return false;
      it->second.deadline = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(kWatchdogMillis);
      return true;
    }

    // Fails every expired request. Called from AppScripting::tick().
    static void sweepExpired() {
      auto now = std::chrono::steady_clock::now();
      std::vector<int> expired;
      {
        base::scoped_lock lock(impl().mutex);
        for (auto& [id, req] : impl().requests)
          if (!req.resolved && req.deadline <= now)
            expired.push_back(id);
      }
      for (auto id : expired)
        fail(id, "format codec did not reply (timeout)");
    }

  private:
    struct Request {
      std::promise<JSON::Value> promise;
      std::chrono::steady_clock::time_point deadline;
      bool resolved = false;
    };

    struct Impl {
      base::mutex mutex;
      int nextId = 1;
      std::map<int, Request> requests;
    };

    static Impl& impl() {
      static Impl instance;
      return instance;
    }

    // Settles the request with a value and erases it (main thread only).
    static bool settle(int id, JSON::Value value) {
      base::scoped_lock lock(impl().mutex);
      auto it = impl().requests.find(id);
      if (it == impl().requests.end() || it->second.resolved)
        return false;
      it->second.resolved = true;
      it->second.promise.set_value(std::move(value));
      impl().requests.erase(it);
      return true;
    }
  };

} // namespace app
