// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_RESOURCE_FINDER_H_INCLUDED
#define APP_RESOURCE_FINDER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <string>
#include <vector>

namespace app {

  // Helper class to find configuration files in different directories
  // in a priority order (e.g. first in the $HOME directory, then in
  // data/ directory, etc.).
  class ResourceFinder {
  public:
    ResourceFinder(bool log = true);

    // Returns the current possible path. You cannot call this
    // function if you haven't call first() or next() before.
    const std::string& filename() const;
    const std::string& defaultFilename() const;

    // Goes to next possible path.
    bool next();

    // Iterates over all possible paths and returns true if the file
    // is exists. Returns the first existent file.
    bool findFirst();

    // These functions add possible full paths to find files.
    void addPath(const std::string& path);
    void includeBinDir(const char* filename);
    void includeDataDir(const char* filename);
    void includeHomeDir(const char* filename);

    // Tries to add the given filename in these locations:
    // For Windows:
    // - If the app is running in portable mode, the filename
    //   will be in the same location as the .exe file.
    // - If the app is installed, the filename will be inside
    //   %AppData% location
    // For Unix-like platforms:
    // - The filename will be in $XDG_CONFIG_HOME/libresprite/
    // or if $XDG_CONFIG_HOME is not defined, $HOME/.config/libresprite/
    void includeUserDir(const char* filename);

    void includeDesktopDir(const char* filename);

    // Returns the first file found or creates the whole directory
    // structure to create the file in its default location.
    std::string getFirstOrCreateDefault();

  private:
    bool m_log;
    std::vector<std::string> m_paths;
    int m_current;
    std::string m_default;

    DISABLE_COPYING(ResourceFinder);
  };

} // namespace app

#endif
