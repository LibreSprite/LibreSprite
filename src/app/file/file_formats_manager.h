// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <vector>

namespace app {

  class FileFormat;

  // A list of file formats. Used by the FileFormatsManager to keep
  // track of all known file extensions supported by ASE.
  typedef std::vector<FileFormat*> FileFormatsList;

  // Manages the list of known formats by ASEPRITE (image file format that can
  // be loaded and/or saved).
  class FileFormatsManager {
  public:
    // Returns a singleton of this class.
    static FileFormatsManager* instance();
    static void destroyInstance();

    virtual ~FileFormatsManager();

    void registerAllFormats();

    // Iterators to access to the list of formats.
    FileFormatsList::iterator begin();
    FileFormatsList::iterator end();

    FileFormat* getFileFormatByExtension(const char* extension) const;

  private:
    // Register one format.
    void registerFormat(FileFormat* fileFormat);

    FileFormatsList m_formats;
  };

} // namespace app
