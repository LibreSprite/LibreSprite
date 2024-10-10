// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/observable.h"
#include "test.h"

class Observer {
public:
  virtual ~Observer() { }
  virtual void on_event_a() { }
  virtual void on_event_b(int a) { }
  virtual void on_event_c(int a, int b) { }
};

class O : public obs::observable<Observer> {
public:
  O() { }
};

class ObserverA : public Observer {
public:
  bool a = false;
  void on_event_a() override { a = true; }
};

class ObserverB : public Observer {
public:
  bool b = false;
  int a_arg = -1;

  void on_event_b(int a) override {
    b = true;
    a_arg = a;
  }
};

class ObserverC : public Observer {
public:
  bool c = false;
  int a_arg = -1;
  int b_arg = -1;

  void on_event_c(int a, int b) override {
    c = true;
    a_arg = a;
    b_arg = b;
  }
};

int main() {
  O o;
  ObserverA a;
  ObserverB b;
  ObserverC c;
  o.add_observer(&a);
  o.add_observer(&b);
  o.add_observer(&c);

  o.notify_observers(&Observer::on_event_a);
  EXPECT_TRUE(a.a);
  EXPECT_FALSE(b.b);
  EXPECT_FALSE(c.c);

  o.notify_observers(&Observer::on_event_b, 1);
  EXPECT_TRUE(b.b);
  EXPECT_FALSE(c.c);
  EXPECT_EQ(1, b.a_arg);

  o.notify_observers(&Observer::on_event_c, 1, 2);
  EXPECT_TRUE(b.b);
  EXPECT_TRUE(c.c);
  EXPECT_EQ(1, c.a_arg);
  EXPECT_EQ(2, c.b_arg);
}
