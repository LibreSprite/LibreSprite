// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/signal.h"
#include "test.h"

struct Entity {
  int a;
  Entity() : a(0) { }
  void set_a(int v) { a = v; }
  int set_a_return_old(int v) {
    int old = a;
    a = v;
    return old;
  }
};

int main() {
  {
    obs::signal<void()> sig;
    int c = 0;
    sig.connect([&](){ ++c; });
    sig.connect([&](){ ++c; });
    sig();
    sig();
    EXPECT_EQ(c, 4);
  }

  {
    obs::signal<void(int)> sig;
    int x = 2;
    sig.connect([&x](int y){ x += y; });
    sig(3);
    sig(4);
    EXPECT_EQ(x, 9);
  }

  {
    obs::signal<void(int, int)> sig;
    int c = 0;
    sig.connect([&](int x, int y){ c = x+y; });
    sig(3, 4);
    EXPECT_EQ(c, 7);
  }

  {
    obs::signal<int()> sig;
    sig.connect([](){ return 1; });
    sig.connect([](){ return 4; });
    int res = sig();
    EXPECT_EQ(res, 4);
  }

  {
    int a=0, b=0;
    double c=0.0;
    obs::signal<void(int, int, double)> sig;
    sig.connect([&](int x, int y, double z) {
                  a = x;
                  b = y;
                  c = z;
                });
    sig(1, 2, 3.4);
    EXPECT_EQ(1, a);
    EXPECT_EQ(2, b);
    EXPECT_EQ(3.4, c);
  }

  {
    obs::signal<void(int)> sig;
    Entity ent;
    sig.connect(&Entity::set_a, &ent);
    EXPECT_EQ(ent.a, 0);
    sig(32);
    EXPECT_EQ(ent.a, 32);
  }

  {
    obs::signal<int(int)> sig;
    Entity ent;
    sig.connect(&Entity::set_a_return_old, &ent);
    ent.a = 2;
    EXPECT_EQ(ent.a, 2);
    int old = sig(32);
    EXPECT_EQ(old, 2);
    EXPECT_EQ(ent.a, 32);
  }
}
