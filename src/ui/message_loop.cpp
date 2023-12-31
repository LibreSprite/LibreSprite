// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/message_loop.h"

#include "ui/manager.h"
#include "she/system.h"

namespace ui {

MessageLoop::MessageLoop(Manager* manager)
  : m_manager(manager)
{
}

void MessageLoop::pumpMessages()
{
  if (m_manager->generateMessages()) {
    m_manager->dispatchMessages();
  } else {
    m_manager->collectGarbage();
  }
  she::instance()->sleep();
}

} // namespace ui
