// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/timer.h"

#include "base/time.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/widget.h"

#include <algorithm>
#include <list>

namespace ui {

  static base::weak_set<Timer> timers; // Registered timers

  static Timer::Shared<Timer> _timer("");

  void Timer::postInject() {
    m_owner = Manager::getDefault()->safePtr;
    timers.insert(safePtr);
  }

  void Timer::postInject(int interval, Widget& owner) {
    m_owner = owner.safePtr;
    m_interval = interval;
    timers.insert(safePtr);
  }

  void Timer::start() {
    m_lastTick = base::current_tick();
    m_running = true;
  }

  void Timer::stop() {
    m_running = false;
  }

  void Timer::tick() {
    onTick();
  }

  void Timer::setInterval(int interval) {
    m_interval = interval;
  }

  void Timer::onTick() {
    // Fire Tick signal.
    Tick();
  }

  void Timer::pollTimers() {
    // Generate messages for timers
    base::tick_t t = base::current_tick();

    for (auto timer : timers) {
      if (timer->isRunning()) {
        int64_t count = ((t - timer->m_lastTick) / timer->m_interval);
        if (count > 0) {
          timer->m_lastTick += count * timer->m_interval;

          ASSERT(timer->m_owner != nullptr);

          auto msg = std::make_shared<TimerMessage>(count, timer->shared_from_this());
          msg->addRecipient(timer->m_owner);
          Manager::getDefault()->enqueueMessage(msg);
        }
      }
    }
  }

} // namespace ui
