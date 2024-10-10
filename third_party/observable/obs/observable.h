// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OBS_OBSERVABLE_H_INCLUDED
#define OBS_OBSERVABLE_H_INCLUDED
#pragma once

#include "obs/observers.h"

namespace obs {

template<typename Observer>
class observable {
public:

  void add_observer(Observer* observer) {
    m_observers.add_observer(observer);
  }

  void remove_observer(Observer* observer) {
    m_observers.remove_observer(observer);
  }

  void notify_observers(void (Observer::*method)()) {
    m_observers.notify_observers(method);
  }

  template<typename ...Args>
  void notify_observers(void (Observer::*method)(Args...), Args ...args) {
    m_observers.template notify_observers<Args...>(method, std::forward<Args>(args)...);
  }

private:
  observers<Observer> m_observers;
};

} // namespace obs

#endif
