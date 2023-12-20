// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/message_loop.h"

#include <chrono>
#include <thread>
#include "ui/manager.h"

namespace ui {

MessageLoop::MessageLoop(Manager* manager)
  : m_manager(manager)
{
}

void MessageLoop::pumpMessages()
{
  using namespace std::chrono_literals;
  auto start = std::chrono::high_resolution_clock::now();

  if (m_manager->generateMessages()) {
    m_manager->dispatchMessages();
  }
  else {
    m_manager->collectGarbage();
  }

  auto end = std::chrono::high_resolution_clock::now();

  // If the dispatching of messages was faster than 10 milliseconds,
  // it means that the process is not using a lot of CPU, so we can
  // wait the difference to cover those 10 milliseconds
  // sleeping. With this code we can avoid 100% CPU usage.
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  if (elapsed < 15ms)
    std::this_thread::sleep_for(15ms - elapsed);
}

} // namespace ui
