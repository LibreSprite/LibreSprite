// Observable Library
// Copyright (c) 2020 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/signal.h"
#include "test.h"

int main() {
  static_assert(true == obs::is_callable_without_args<void()>::value, "");
  static_assert(true == obs::is_callable_without_args<int()>::value, "");
  static_assert(false == obs::is_callable_without_args<void(int)>::value, "");
  static_assert(false == obs::is_callable_without_args<int(int)>::value, "");

  int i = 0;
  obs::signal<void()   > a; a.connect([&i]{ ++i; });
  obs::signal<void(int)> b; b.connect([&i]{ ++i; });
  obs::signal<int ()   > c; c.connect([&i]{ return ++i; });
  obs::signal<int (int)> d; d.connect([&i]{ return ++i; });

  a();
  b(1);
  c();
  d(1);
  EXPECT_EQ(4, i);
}
