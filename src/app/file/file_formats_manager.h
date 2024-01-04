// LibreSprite
// Copyright (C) 2001-2015  David Capello
// Copyright (c) 2024 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/file/file_format.h"
#include "base/injection.h"
#include <vector>

namespace app {
  // A list of file formats. Used by the FileFormatsManager to keep
  // track of all known file extensions supported by ASE.
  typedef std::vector<inject<FileFormat>> FileFormatsList;

  // Manages the list of known formats by ASEPRITE (image file format that can
  // be loaded and/or saved).
  class FileFormatsManager {
  public:
    // Returns a singleton of this class.
    static FileFormatsManager* instance();
    static void destroyInstance();

    std::vector<FileFormat*> support(int);

    FileFormat* getFileFormatByExtension(const char* extension);

  private:
    FileFormatsList m_formats = FileFormat::getAll();
  };

} // namespace app
