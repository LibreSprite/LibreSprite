// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/crash/raw_images_as.h"
#include "base/disable_copying.h"
#include "base/process.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace app {
  class Document;

namespace crash {

  // A class to record/restore session information.
  class Session {
  public:
    class Backup {
    public:
      Backup(const std::string& dir);
      const std::string& dir() const { return m_dir; }
      const std::string& description() const { return m_desc; }
    private:
      std::string m_dir;
      std::string m_desc;
    };
    typedef std::vector<Backup*> Backups;

    Session(const std::string& path);
    ~Session();

    std::string name() const;
    const Backups& backups();

    bool isRunning();
    bool isEmpty();

    void create(base::pid pid);
    void removeFromDisk();

    void saveDocumentChanges(app::Document* doc);
    void removeDocument(app::Document* doc);

    void restoreBackup(Backup* backup);
    void restoreRawImages(Backup* backup, RawImagesAs as);
    void deleteBackup(Backup* backup);

  private:
    void loadPid();
    std::string pidFilename() const;
    std::string verFilename() const;
    void deleteDirectory(const std::string& dir);
    void fixFilename(app::Document* doc);

    base::pid m_pid;
    std::string m_path;
    std::fstream m_log;
    std::fstream m_pidFile;
    Backups m_backups;

    DISABLE_COPYING(Session);
  };

typedef std::shared_ptr<Session> SessionPtr;

} // namespace crash
} // namespace app
