// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/send_crash.h"

#include "app/app.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/launcher.h"

#include "send_crash.xml.h"

namespace app {

#ifdef _WIN32
static const char* kDefaultCrashName = PACKAGE_AND_VERSION "-crash.dmp";
#endif

std::string memory_dump_filename()
{
#ifdef _WIN32

  app::ResourceFinder rf;
  rf.includeUserDir(kDefaultCrashName);
  return rf.getFirstOrCreateDefault();

#else
  return "";
#endif
}

void SendCrash::search()
{
#ifdef _WIN32
  m_dumpFilename = memory_dump_filename();

  if (base::is_file(m_dumpFilename)) {
    App::instance()->showNotification(this);
  }
#endif
}

std::string SendCrash::notificationText()
{
  return "Report last crash";
}

void SendCrash::notificationClick()
{
  if (m_dumpFilename.empty()) {
    ui::Alert::show("Crash Report<<Nothing to report||&OK");
    return;
  }

  app::gen::SendCrash dlg;

  // The current version is a "development" version if the VERSION
  // macro contains the "dev" word.
  bool isDev = (std::string(VERSION).find("dev") != std::string::npos);
  if (isDev) {
    dlg.official()->setVisible(false);
    dlg.devFilename()->setText(m_dumpFilename);
    dlg.devFilename()->Click.connect(base::Bind(&SendCrash::onClickDevFilename, this));
  }
  else {
    dlg.dev()->setVisible(false);
    dlg.filename()->setText(m_dumpFilename);
    dlg.filename()->Click.connect(base::Bind(&SendCrash::onClickFilename, this));
  }

  dlg.openWindowInForeground();
  if (dlg.closer() == dlg.deleteFile()) {
    try {
      base::delete_file(m_dumpFilename);
      m_dumpFilename = "";
    }
    catch (const std::exception& ex) {
      ui::Alert::show("Error<<%s||&OK", ex.what());
    }
  }
}

void SendCrash::onClickFilename()
{
  base::launcher::open_folder(m_dumpFilename);
}

void SendCrash::onClickDevFilename()
{
  base::launcher::open_file(m_dumpFilename);
}

} // namespace app
