// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/mcp/mcp_ui_bridge.h"

#include "she/system.h"
#include "ui/manager.h"
#include "ui/timer.h"

#include <chrono>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

#ifdef USE_SDL2_BACKEND
  #include <SDL.h>
#endif

namespace app {
namespace mcp {
namespace {

struct WorkItem {
  std::function<void()> fn;
  std::string* error = nullptr;
  std::mutex* mu = nullptr;
  std::condition_variable* cv = nullptr;
  bool* done = nullptr;
};

std::mutex g_queueMu;
std::queue<WorkItem> g_queue;
std::unique_ptr<ui::Timer> g_timer;
bool g_draining = false;

void wakeUi()
{
#ifdef USE_SDL2_BACKEND
  SDL_Event ev;
  SDL_zero(ev);
  ev.type = SDL_USEREVENT;
  SDL_PushEvent(&ev);
#endif
  // Also nudge she::gfx path used by other async work.
  if (she::instance() && !she::instance()->isGfxThread()) {
    she::instance()->gfx([]{}, false);
  }
}

void drainQueue()
{
  if (g_draining)
    return;
  g_draining = true;

  for (;;) {
    WorkItem item;
    {
      std::lock_guard<std::mutex> lock(g_queueMu);
      if (g_queue.empty())
        break;
      item = std::move(g_queue.front());
      g_queue.pop();
    }

    std::string error;
    try {
      item.fn();
    }
    catch (const std::exception& e) {
      error = e.what();
    }
    catch (...) {
      error = "Unknown UI-thread error";
    }

    if (item.error)
      *item.error = error;
    if (item.mu && item.cv && item.done) {
      {
        std::lock_guard<std::mutex> lock(*item.mu);
        *item.done = true;
      }
      item.cv->notify_one();
    }
  }

  g_draining = false;
}

} // namespace

void initUiBridge()
{
  if (g_timer)
    return;
  if (!ui::Manager::getDefault())
    return;

  g_timer = std::make_unique<ui::Timer>(16);
  g_timer->Tick.connect([]{ drainQueue(); });
  g_timer->start();
}

std::string callOnUiThread(const std::function<void()>& fn)
{
  // Already on UI/gfx thread: run inline.
  if (she::instance() && she::instance()->isGfxThread()) {
    try {
      fn();
      return {};
    }
    catch (const std::exception& e) {
      return e.what();
    }
    catch (...) {
      return "Unknown UI-thread error";
    }
  }

  std::string error;
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;

  {
    std::lock_guard<std::mutex> lock(g_queueMu);
    g_queue.push(WorkItem{fn, &error, &mu, &cv, &done});
  }
  wakeUi();

  // Also try she::gfx as a secondary delivery path (when refresh runs).
  if (she::instance()) {
    she::instance()->gfx([]{ drainQueue(); }, false);
  }

  {
    std::unique_lock<std::mutex> lock(mu);
    if (!cv.wait_for(lock, std::chrono::seconds(30), [&]{ return done; }))
      return "Timed out waiting for UI thread (is LibreSprite's window running?)";
  }
  return error;
}

} // namespace mcp
} // namespace app
