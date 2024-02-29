// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_formats_manager.h"

#include "app/file/format_options.h"
#include "base/string.h"

#include <algorithm>
#include <cstring>
#include <memory>

namespace app {
static std::unique_ptr<FileFormatsManager> singleton;

// static
FileFormatsManager* FileFormatsManager::instance()
{
  if (!singleton)
    singleton.reset(new FileFormatsManager());
  return singleton.get();
}

// static
void FileFormatsManager::destroyInstance()
{
  singleton.reset();
}

std::vector<FileFormat*> FileFormatsManager::support(int flags)
{
  std::vector<FileFormat*> pick;
  for (auto& ff : m_formats) {
    if (ff->support(flags))
      pick.push_back(ff.get());
  }
  std::sort(pick.begin(), pick.end(), [](auto a, auto b){return a->listPriority() < b->listPriority();});
  return pick;
}

FileFormat* FileFormatsManager::getFileFormatByExtension(const char* extension)
{
  std::string extensions;
  for (auto& ff : m_formats) {
    extensions = ff->extensions();
    for (auto tok=std::strtok(extensions.data(), ","); tok; tok=std::strtok(NULL, ",")) {
      if (base::utf8_icmp(extension, tok) == 0)
        return ff.get();
    }
  }

  return nullptr;
}

} // namespace app
