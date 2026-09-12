// LibreSprite
// Copyright (C) 2024-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// Timer extension: setTimeout / setInterval / clearTimeout / clearInterval.
// Ported from Delta's src/extensions/Timer.cpp (a pure-C++ sorted
// steady_clock timer vector fired from the per-frame tick pump). It makes
// `await app.yield()` (and any async script) possible: callbacks are invoked
// by AppScripting::tick(), which the 1ms ScriptTimer drives in GUI mode.

#include "delta/Extension.hpp"
#include "di.hpp"
#include "app/script/app_scripting.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <vector>

class TimerExtension : public Extension {
public:
  static inline std::size_t nextID = 1;

  using Timepoint = std::chrono::time_point<std::chrono::steady_clock>;
  struct Timer {
    std::size_t id;
    JSON::Value callback;
    bool repeating{false};
    Timepoint startTime;
    double delay{};
    std::size_t triggerCount{0};
    Timepoint time;
  };

  std::vector<Timer> timers;

  TimerExtension() {
    addFunction("setTimeout") = [this](JSON::Value& callback, double delay) {
      if (!callback.isFunction())
        return JSON::Value{0.0};
      if (delay < 0)
        delay = 0;
      Timer timer;
      timer.id = nextID++;
      timer.callback = callback;
      timer.time = std::chrono::steady_clock::now() +
                   std::chrono::milliseconds(static_cast<int64_t>(delay));
      timers.push_back(timer);
      std::sort(timers.begin(), timers.end(), [](const Timer& a, const Timer& b) {
        return a.time < b.time;
      });
      return JSON::Value{static_cast<double>(timer.id)};
    };

    addFunction("setInterval") = [this](JSON::Value& callback, double delay) {
      if (!callback.isFunction())
        return JSON::Value{0.0};
      // Clamp to the tick granularity so a 0-delay interval cannot busy-loop
      // inside a single tick() call.
      if (delay < 1)
        delay = 1;
      Timer timer;
      timer.id = nextID++;
      timer.callback = callback;
      timer.repeating = true;
      timer.startTime = std::chrono::steady_clock::now();
      timer.delay = delay;
      timer.time = timer.startTime +
                   std::chrono::milliseconds(static_cast<int64_t>(delay));
      timers.push_back(timer);
      std::sort(timers.begin(), timers.end(), [](const Timer& a, const Timer& b) {
        return a.time < b.time;
      });
      return JSON::Value{static_cast<double>(timer.id)};
    };

    addFunction("clearTimeout") = [this](double id) {
      auto uid = static_cast<std::size_t>(id);
      auto it = std::remove_if(timers.begin(), timers.end(),
                               [uid](const Timer& t) { return t.id == uid; });
      if (it != timers.end())
        timers.erase(it, timers.end());
    };

    addFunction("clearInterval") = [this](double id) {
      auto uid = static_cast<std::size_t>(id);
      auto it = std::remove_if(timers.begin(), timers.end(),
                               [uid](const Timer& t) { return t.id == uid; });
      if (it != timers.end())
        timers.erase(it, timers.end());
    };
  }

  std::chrono::milliseconds tick() override {
    auto now = std::chrono::steady_clock::now();
    while (!timers.empty() && timers.front().time <= now) {
      Timer timer = std::move(timers.front());
      timers.erase(timers.begin());
      if (timer.repeating) {
        timer.triggerCount++;
        timer.time = timer.startTime + std::chrono::milliseconds(
                        static_cast<int64_t>(timer.delay * (timer.triggerCount + 1)));
        // Guard against the busy-loop: never re-arm in the past.
        if (timer.time <= now)
          timer.time = now + std::chrono::milliseconds(1);
        timers.push_back(timer);
        std::sort(timers.begin(), timers.end(), [](const Timer& a, const Timer& b) {
          return a.time < b.time;
        });
      }
      JSON::Array args;
      // A JS exception thrown by the callback is returned (not propagated)
      // as an {exception: "..."} object by the delta wrapper.
      auto ret = timer.callback(args);
      if (ret.isObject() && ret.object().contains("exception"))
        app::AppScripting::consolePrint(ret["exception"].string());
    }
    if (timers.empty())
      return std::chrono::milliseconds(0);
    auto delta = timers.front().time - std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta);
    return ms < std::chrono::milliseconds(1) ? std::chrono::milliseconds(1) : ms;
  }

  bool keepAlive() override {
    return !timers.empty();
  }
};

static di::provide<Extension, TimerExtension> timerExt{"timer"};
