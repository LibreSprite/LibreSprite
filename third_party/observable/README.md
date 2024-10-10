Observable Library
==================

*Copyright (C) 2016-2021 David Capello*

[![build](https://github.com/dacap/observable/actions/workflows/build.yml/badge.svg)](https://github.com/dacap/observable/actions/workflows/build.yml)
[![MIT Licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)

Library to use the observer pattern in C++11 programs with
observable/observer classes or signals/slots.

Features
--------

* Generate an observable notification/signal from multiple threads
* Add/remove observers/slots from multiple threads
* Erase/disconnect an observer/slot from the same observable notification/signal
* Reconnect an observer in the same notification

Observable
----------

An observable `Widget`:

```cpp
#include "obs.h"

class WidgetObserver {
public:
  virtual ~WidgetObserver() = 0;
  virtual void onClick() { }
};

class Widget : public obs::observable<WidgetObserver> {
public:
  void processClick() {
    notify_observers(&WidgetObserver::onClick);
  }
};
```

An example

```cpp
#include "obs.h"

class ObserveClick : public WidgetObserver {
public:
  void onClick() override {
    // Do something...
  }
};

...
ObserveClick observer;
Widget button;
button.add_observer(&observer);
```

Signal
------

```cpp
#include "obs.h"

int main() {
  obs::signal<void (int, int)> sig;
  sig.connect([](int x, int y){ ... });
  sig(1, 2); // Generate signal
}
```

Tested Compilers
----------------

* Visual Studio 2015
* Xcode 7.3.1 (`-std=c++11`)
* GCC 4.8.4 (`-std=c++11`)
