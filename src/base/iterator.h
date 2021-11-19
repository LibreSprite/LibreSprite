// LibreSprite Base Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace base {

template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin (reversion_wrapper<T> w) -> decltype(w.iterable.rbegin()) { return w.iterable.rbegin(); }

template <typename T>
auto end (reversion_wrapper<T> w) -> decltype(w.iterable.rend()) { return w.iterable.rend(); }

template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

template <typename T>
struct const_reversion_wrapper { const T& iterable; };

template <typename T>
auto begin (const_reversion_wrapper<T> w) -> decltype(w.iterable.crbegin()) { return w.iterable.crbegin(); }

template <typename T>
auto end (const_reversion_wrapper<T> w) -> decltype(w.iterable.crend()) { return w.iterable.crend(); }

template <typename T>
const_reversion_wrapper<T> reverse (const T& iterable) { return { iterable }; }

}
