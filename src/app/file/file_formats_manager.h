// LibreSprite
// Copyright (C) 2001-2015  David Capello
// Copyright (c) 2024 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/file/file_format.h"
#include "app/file/script_file_format.h"
#include "base/injection.h"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace app {
  // A list of file formats. Used by the FileFormatsManager to keep
  // track of all known file extensions supported by ASE.
  typedef std::vector<inject<FileFormat>> FileFormatsList;

  // Keeps a script-registered format alive: the format instance and the
  // Provides that publishes it to the DI registry (keyed by the script's
  // canonical path). The manager owns one per registration. Provides is
  // declared before format (members initialize in declaration order) so it
  // captures the instance pointer before the unique_ptr is moved away.
  struct ScriptFormatRegistration {
    ScriptFormatRegistration(std::unique_ptr<ScriptFileFormat> format,
                             const std::string& path)
    : provides(format.get(), path, {"script"})
    , format(std::move(format))
    , path(path)
    {
    }

    FileFormat::Provides provides;
    std::unique_ptr<ScriptFileFormat> format;
    std::string path;
  };

  // Manages the list of known formats by ASEPRITE (image file format that can
  // be loaded and/or saved).
  class FileFormatsManager {
  public:
    // Returns a singleton of this class.
    static FileFormatsManager* instance();
    static void destroyInstance();

    // Shared-locked (m_formatsMutex): FileOp workers (open/save jobs,
    // thumbnails) call it while the main thread registers formats.
    std::vector<FileFormat*> support(int);

    // Shared-locked (m_formatsMutex), as support().
    FileFormat* getFileFormatByExtension(const char* extension);

    // Registers a script-implemented format (file.registerFormat), keyed
    // by the calling script's canonical path; re-registering for the same
    // path retires the prior entry (kept alive until exit: a FileOp may
    // hold its raw pointer mid-operation). Takes the exclusive
    // m_formatsMutex lock against the shared-locked readers above.
    void registerScriptFormat(const std::string& scriptPath,
                              const std::string& name,
                              const std::string& extensions,
                              bool load, bool save);

  private:
    FileFormatsList m_formats = FileFormat::getAll();
    std::shared_mutex m_formatsMutex;
    // Declared after m_formats: members destroy in reverse declaration
    // order, and the holders own the instances the script injects in
    // m_formats point to (the built-in injects own their own instances).
    std::vector<std::unique_ptr<ScriptFormatRegistration>> m_scriptFormats;
    // Replaced script registrations, kept alive until exit: a FileOp may
    // hold a raw pointer mid-operation (it owns no reference), and a
    // ScriptFileFormat is immutable after construction.
    std::vector<std::unique_ptr<ScriptFormatRegistration>> m_retired;
  };

} // namespace app
