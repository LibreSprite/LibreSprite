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
  virtual void on_event() { }
};

class Observable : public obs::observable<Observer> {
public:
  Observable() { }
};

Observable object;

class Reconnect : public Observer {
public:
  void on_event() override {
    // Here we reconnect this observer at the end of the observers
    // list, the library should avoid an infinite loop iterating
    // the same observer again and again.
    object.remove_observer(this);
    object.add_observer(this);
  }
};

int main() {
  Reconnect r;
  object.add_observer(&r);
  object.notify_observers(&Observer::on_event);
}
