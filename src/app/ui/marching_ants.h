// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/connection.h"
#include "ui/timer.h"
#include "ui/manager.h"
#include <cmath>

namespace app {

  class MarchingAnts {
  public:
    MarchingAnts() {
      m_scopedConn = m_timer->Tick.connect(&MarchingAnts::onTick, this);
    }

    ~MarchingAnts() {
      m_timer->stop();
    }

  protected:
    virtual void onDrawMarchingAnts() = 0;

    int getMarchingAntsOffset() const {
      return m_offset;
    }

    bool isMarchingAntsRunning() const {
      return m_timer->isRunning();
    }

    void startMarchingAnts() {
      m_timer->start();
    }

    void stopMarchingAnts() {
      m_timer->stop();
    }

  private:
    void onTick() {
      m_offset = ((m_offset+1) % 8);
      onDrawMarchingAnts();
    }

    inject<ui::Timer> m_timer = ui::Timer::create(100);
    int m_offset = 0;
    base::ScopedConnection m_scopedConn;
  };

} // namespace app
