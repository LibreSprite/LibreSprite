// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/signal.h"
#include "test.h"

class A {
public:
  A(obs::signal<void()>& signal, int& value) : m_value(value) {
    m_conn = signal.connect(&A::on_signal, this);
  }

private:
  void on_signal() { ++m_value; }

  obs::scoped_connection m_conn;
  int& m_value;
};

int main() {
  obs::signal<void()> signal;
  int value = 0;
  {
    A a(signal, value);
    EXPECT_EQ(value, 0);
    signal();
    EXPECT_EQ(value, 1);
  }
  signal();
  EXPECT_EQ(value, 1);
}
