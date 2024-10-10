// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OBS_SIGNAL_H_INCLUDED
#define OBS_SIGNAL_H_INCLUDED
#pragma once

#include "obs/connection.h"
#include "obs/safe_list.h"
#include "obs/slot.h"

#include <memory>
#include <functional>
#include <type_traits>

namespace obs {

class signal_base {
public:
  virtual ~signal_base() { }
  virtual void disconnect_slot(slot_base* slot) = 0;
};

// Signal for any kind of functions
template<typename Callable>
class signal { };

template<typename R, typename...Args>
class signal<R(Args...)> : public signal_base {
public:
  typedef R result_type;
  typedef slot<R(Args...)> slot_type;
  typedef safe_list<slot_type> slot_list;

  signal() { }
  ~signal() {
    for (auto slot : m_slots)
      delete slot;
  }

  signal(const signal&) { }
  signal& operator=(const signal&) { return *this; }

  connection add_slot(slot_type* s) {
    m_slots.push_back(s);
    return connection(this, s);
  }

  template<typename Function>
  connection connect(Function&& f) {
    return add_slot(new slot_type(std::forward<Function>(f)));
  }

  template<class Class>
  connection connect(result_type (Class::*m)(Args...args), Class* t) {
    return add_slot(new slot_type(
                      [=](Args...args) -> result_type {
                        return (t->*m)(std::forward<Args>(args)...);
                      }));
  }

  virtual void disconnect_slot(slot_base* slot) override {
    m_slots.erase(static_cast<slot_type*>(slot));
  }

  template<typename...Args2>
  typename std::enable_if<!std::is_void<R>::value, R>::type
  operator()(Args2&&...args) {
    R result = R();
    for (auto slot : m_slots)
      if (slot)
        result = (*slot)(std::forward<Args2>(args)...);
    return result;
  }

protected:
  slot_list m_slots;
};

template<typename...Args>
class signal<void(Args...)> : public signal_base {
public:
  typedef slot<void(Args...)> slot_type;
  typedef safe_list<slot_type> slot_list;

  signal() = default;

  ~signal() {
    for (auto slot : *m_slots)
      delete slot;
    m_slots->clear();
  }

  signal(const signal&) { }
  signal& operator=(const signal&) { return *this; }

  connection add_slot(slot_type* s) {
    m_slots->push_back(s);
    return connection(this, s);
  }

  template<typename Function>
  connection connect(Function&& f) {
    return add_slot(new slot_type(std::forward<Function>(f)));
  }

  template<class Class>
  connection connect(void (Class::*m)(Args...args), Class* t) {
    return add_slot(new slot_type(
                      [=](Args...args) {
                        (t->*m)(std::forward<Args>(args)...);
                      }));
  }

  virtual void disconnect_slot(slot_base* slot) override {
    m_slots->erase(static_cast<slot_type*>(slot));
  }

  template<typename...Args2>
  void operator()(Args2&&...args) {
    auto slots = m_slots;
    for (auto slot : *slots)
      if (slot)
        (*slot)(std::forward<Args2>(args)...);
  }

protected:
  std::shared_ptr<slot_list> m_slots = std::make_shared<slot_list>();
};

} // namespace obs

#endif
