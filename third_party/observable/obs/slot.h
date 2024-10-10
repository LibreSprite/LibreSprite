// Observable Library
// Copyright (c) 2016-2020 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OBS_SLOT_H_INCLUDED
#define OBS_SLOT_H_INCLUDED
#pragma once

#include <functional>
#include <type_traits>

namespace obs {

template<typename T>
struct is_callable_without_args : std::is_convertible<T, std::function<void()>> { };

class slot_base {
public:
  slot_base() { }
  virtual ~slot_base() { }

  // Disable copy
  slot_base(const slot_base&) = delete;
  slot_base& operator=(const slot_base&) = delete;
};

// Generic slot
template<typename Callable>
class slot { };

template<typename R, typename...Args>
class slot<R(Args...)> : public slot_base {
public:
  template<typename F,
           typename = typename std::enable_if<(sizeof...(Args) == 0 ||
                                               !is_callable_without_args<F>::value)>::type>
  slot(F&& f) : f(std::forward<F>(f)) { }

  template<typename G,
           typename = typename std::enable_if<(sizeof...(Args) != 0 &&
                                               is_callable_without_args<G>::value)>::type>
  slot(G g) : f([g](Args...) -> R { return g(); }) { }

  slot(const slot& s) { (void)s; }
  virtual ~slot() { }

  template<typename...Args2>
  R operator()(Args2&&...args) {
    assert(f);
    return f(std::forward<Args2>(args)...);
  }

private:
  std::function<R(Args...)> f;
};

template<typename...Args>
class slot<void(Args...)> : public slot_base {
public:
  template<typename F,
           typename = typename std::enable_if<(sizeof...(Args) == 0 ||
                                               !is_callable_without_args<F>::value)>::type>
  slot(F&& f) : f(std::forward<F>(f)) { }

  template<typename G,
           typename = typename std::enable_if<(sizeof...(Args) != 0 &&
                                               is_callable_without_args<G>::value)>::type>
  slot(G g) : f([g](Args...){ g(); }) { }

  slot(const slot& s) { (void)s; }
  virtual ~slot() { }

  template<typename...Args2>
  void operator()(Args2&&...args) {
    assert(f);
    f(std::forward<Args2>(args)...);
  }

private:
  std::function<void(Args...)> f;
};

} // namespace obs

#endif
