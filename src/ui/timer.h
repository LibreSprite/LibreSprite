// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/safe_ptr.h"
#include "base/signal.h"
#include "base/time.h"
#include "ui/manager.h"
#include <memory>

namespace ui {

  class Widget;

  class Timer : public Injectable<Timer>, public std::enable_shared_from_this<Timer> {
  protected:
    Timer() = default;

  public:
    void postInject() override;
    virtual void postInject(int interval, Widget& owner = *Manager::getDefault());

    int interval() const {
      return m_interval;
    }

    void setInterval(int interval);

    bool isRunning() const {
      return m_running;
    }

    void start();
    void stop();

    void tick();

    base::Signal0<void> Tick;

    static void pollTimers();

  protected:
    virtual void onTick();

  public:
    base::safe_ptr<Timer> safePtr{this};
    base::safe_ptr<Widget> m_owner;
    int m_interval = 1;
    bool m_running = 0;
    base::tick_t m_lastTick = 0;

    DISABLE_COPYING(Timer);
  };

} // namespace ui
