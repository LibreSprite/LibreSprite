// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OBS_OBSERVERS_H_INCLUDED
#define OBS_OBSERVERS_H_INCLUDED
#pragma once

#include "obs/safe_list.h"

namespace obs {

template<typename T>
class observers {
public:
  typedef T observer_type;
  typedef safe_list<observer_type> list_type;
  typedef typename list_type::iterator iterator;

  bool empty() const { return m_observers.empty(); }
  std::size_t size() const { return m_observers.size(); }

  void add_observer(observer_type* observer) {
    m_observers.push_back(observer);
  }

  void remove_observer(observer_type* observer) {
    m_observers.erase(observer);
  }

  template<typename ...Args>
  void notify_observers(void (observer_type::*method)(Args...), Args ...args) {
    for (auto observer : m_observers) {
      if (observer)
        (observer->*method)(std::forward<Args>(args)...);
    }
  }

private:
  list_type m_observers;
};

} // namespace obs

#endif
