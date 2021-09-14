// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdio.h>

#include "base/bind.h"
#include "base/memory.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"

namespace app {

using namespace ui;

static Window* wid_console = NULL;
static Widget* wid_view = NULL;
static Widget* wid_textbox = NULL;
static Widget* wid_cancel = NULL;
static int console_counter = 0;
static bool console_locked;
static bool want_close_flag = false;

Console::Console(Context* ctx)
  : m_withUI(false)
{
  if (ctx)
    m_withUI = (ctx->isUIAvailable());
  else
    m_withUI =
      (App::instance()->isGui() &&
       Manager::getDefault() &&
       Manager::getDefault()->getDisplay());

  if (!m_withUI)
    return;

  console_counter++;
  if (wid_console || console_counter > 1)
    return;

  Window* window = new Window(Window::WithTitleBar, "Errors Console");
  Grid* grid = new Grid(1, false);
  View* view = new View();
  TextBox* textbox = new TextBox("", WORDWRAP);
  Button* button = new Button("&Cancel");

  if (!grid || !textbox || !button)
    return;

  // The "button" closes the console
  button->Click.connect(base::Bind<void>(&Window::closeWindow, window, button));

  view->attachToView(textbox);

  button->setMinSize(gfx::Size(60, 0));

  grid->addChildInCell(view, 1, 1, HORIZONTAL | VERTICAL);
  grid->addChildInCell(button, 1, 1, CENTER);
  window->addChild(grid);

  view->setVisible(false);
  button->setFocusMagnet(true);
  view->setExpansive(true);

  wid_console = window;
  wid_view = view;
  wid_textbox = textbox;
  wid_cancel = button;
  console_locked = false;
  want_close_flag = false;
}

Console::~Console()
{
  if (!m_withUI)
    return;

  console_counter--;

  if ((wid_console) && (console_counter == 0)) {
    if (console_locked
        && !want_close_flag
        && wid_console->isVisible()) {
      // Open in foreground
      wid_console->openWindowInForeground();
    }

    delete wid_console;         // window
    wid_console = NULL;
    want_close_flag = false;
  }
}

void Console::printf(const char* format, ...)
{
  char buf[4096];               // TODO warning buffer overflow
  va_list ap;

  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  if (!m_withUI || !wid_console) {
    fputs(buf, stdout);
    fflush(stdout);
    return;
  }

  // Open the window
  if (!wid_console->isVisible()) {
    wid_console->openWindow();
    ui::Manager::getDefault()->invalidate();
  }

  /* update the textbox */
  if (!console_locked) {
    console_locked = true;

    wid_view->setVisible(true);

    wid_console->remapWindow();
    wid_console->setBounds(gfx::Rect(0, 0, ui::display_w()*9/10, ui::display_h()*6/10));
    wid_console->centerWindow();
    wid_console->invalidate();
  }

  const std::string& text = wid_textbox->text();

  std::string final;
  if (!text.empty())
    final += text;
  final += buf;

  wid_textbox->setText(final.c_str());
}

// static
void Console::showException(const std::exception& e)
{
  Console console;
  if (typeid(e) == typeid(std::bad_alloc))
    console.printf("There is not enough memory to complete the action.");
  else
    console.printf("A problem has occurred.\n\nDetails:\n%s\n", e.what());
}

} // namespace app
