// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/connection.h"
#include "ui/timer.h"

#include <cmath>

namespace app {

  class AnimatedWidget {
  public:
    AnimatedWidget() {
      m_scopedConn = m_timer->Tick.connect(&AnimatedWidget::onTick, this);
    }

    ~AnimatedWidget() {
      m_timer->stop();
    }

    // For each animation frame
    virtual void onAnimationStart() { }
    virtual void onAnimationStop(int animation) { }
    virtual void onAnimationFrame() { }

  protected:
    void startAnimation(int animation, int lifespan) {
      // Stop previous animation
      if (m_animation)
        stopAnimation();

      m_animation = animation;
      m_animationTime = 0;
      m_animationLifespan = lifespan;
      m_timer->start();

      onAnimationStart();
    }

    void stopAnimation() {
      int animation = m_animation;
      m_animation = 0;
      m_timer->stop();

      onAnimationStop(animation);
    }

    int animation() const {
      return m_animation;
    }

    double animationTime() const {
      return double(m_animationTime) / double(m_animationLifespan);
    }

    double ease(double t) {
      return (1.0 - std::pow(1.0 - t, 2));
    }

    double inbetween(double x0, double x1, double t) {
      return x0 + (x1-x0)*ease(t);
    }

  private:
    void onTick() {
      if (m_animation) {
        if (m_animationTime == m_animationLifespan)
          stopAnimation();
        else
          ++m_animationTime;

        onAnimationFrame();
      }
    }

    inject<ui::Timer> m_timer = ui::Timer::create(1000/60);
    int m_animation = 0;
    int m_animationTime;
    int m_animationLifespan;
    base::ScopedConnection m_scopedConn;
  };

} // namespace app
