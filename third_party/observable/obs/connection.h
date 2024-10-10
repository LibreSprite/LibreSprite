// Observable Library
// Copyright (c) 2016-2021 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OBS_CONNETION_H_INCLUDED
#define OBS_CONNETION_H_INCLUDED
#pragma once

namespace obs {

class signal_base;
class slot_base;

class connection {
public:
  connection() : m_signal(nullptr),
                 m_slot(nullptr) {
  }

  connection(signal_base* sig,
             slot_base* slot) :
    m_signal(sig),
    m_slot(slot) {
  }

  void disconnect();

  operator bool() {
    return (m_slot != nullptr);
  }

private:
  signal_base* m_signal;
  slot_base* m_slot;
};

class scoped_connection {
public:
  scoped_connection() {
  }

  scoped_connection(const connection& conn) : m_conn(conn) {
  }

  scoped_connection& operator=(const connection& conn) {
    m_conn.disconnect();
    m_conn = conn;
    return *this;
  }

  ~scoped_connection() {
    m_conn.disconnect();
  }

  // Just in case that we want to disconnect the signal in the middle
  // of the scope.
  void disconnect() {
    m_conn.disconnect();
  }

private:
  connection m_conn;
};

} // namespace obs

#endif
