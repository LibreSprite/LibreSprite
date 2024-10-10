// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/connection.h"
#include "obs/signal.h"

namespace obs {

void connection::disconnect() {
  if (!m_slot)
    return;

  assert(m_signal);
  if (m_signal)
    m_signal->disconnect_slot(m_slot);

  delete m_slot;
  m_slot = nullptr;
}

} // namespace obs
