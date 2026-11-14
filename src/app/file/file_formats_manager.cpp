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
  std::shared_lock<std::shared_mutex> lock(m_formatsMutex);
  for (auto& ff : m_formats) {
    if (ff->support(flags))
      pick.push_back(ff.get());
  }
  std::sort(pick.begin(), pick.end(), [](auto a, auto b){return a->listPriority() < b->listPriority();});
  return pick;
}

FileFormat* FileFormatsManager::getFileFormatByExtension(const char* extension)
{
  std::shared_lock<std::shared_mutex> lock(m_formatsMutex);
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

void FileFormatsManager::registerScriptFormat(const std::string& scriptPath,
                                              const std::string& name,
                                              const std::string& extensions,
                                              bool load, bool save)
{
  // Exclusive: the FileOp workers read m_formats through the shared lock.
  std::unique_lock<std::shared_mutex> lock(m_formatsMutex);

  // The prior registration for this script path (if any), as a raw
  // pointer: the pushes below may reallocate m_scriptFormats.
  FileFormat* oldFormat = nullptr;
  for (auto& reg : m_scriptFormats)
    if (reg->path == scriptPath)
      oldFormat = reg->format.get();

  // Publish the new registration first so the registry key is live
  // throughout; the stale Provides then refuses to erase the fresh entry
  // (its destructor only removes entries it still owns).
  m_scriptFormats.push_back(std::make_unique<ScriptFormatRegistration>(
    std::make_unique<ScriptFileFormat>(scriptPath, name, extensions, load, save),
    scriptPath));
  m_formats.emplace_back(scriptPath);

  if (oldFormat) {
    m_formats.erase(std::find_if(m_formats.begin(), m_formats.end(),
      [oldFormat](const auto& ff){ return ff.get() == oldFormat; }));
    // Retire the old registration instead of destroying it: a FileOp may
    // be mid-operation holding its raw pointer (it owns no reference), so
    // the instance stays alive until exit (it is not picked for new
    // operations: its entry left m_formats above).
    auto oldIt = std::find_if(m_scriptFormats.begin(), m_scriptFormats.end(),
      [oldFormat](const auto& reg){ return reg->format.get() == oldFormat; });
    m_retired.push_back(std::move(*oldIt));
    m_scriptFormats.erase(oldIt);
  }
}

} // namespace app
