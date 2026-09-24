// Copyright (C) 2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#if __has_include(<archive.h>)

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"

#include <archive.h>
#include <archive_entry.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace app
{

namespace extension_format_detail
{

// Rejects absolute paths (Unix, Windows drive-letter, or UNC) and any path
// containing a ".." component, so an archive entry can never write outside
// the destination directory it's being extracted to (zip-slip).
inline bool isSafeArchiveEntryPath(const std::string& fileName)
{
  if (fileName.empty())
    return false;
  if (fileName[0] == '/' || fileName[0] == '\\')
    return false;
  if (fileName.size() >= 2 && fileName[1] == ':')
    return false;

  size_t start = 0;
  while (start <= fileName.size())
  {
    size_t end = fileName.find_first_of("/\\", start);
    if (end == std::string::npos)
      end = fileName.size();
    if (fileName.compare(start, end - start, "..") == 0)
      return false;
    start = end + 1;
  }
  return true;
}

} // namespace extension_format_detail

// Reads a 7z/gnutar/rar/tar/zip archive and extracts it into a directory.
// Entries that would escape the destination directory (absolute paths or
// ".." components) are rejected, and symlinks/hardlinks/sockets are skipped
// instead of extracted, since their target could point outside it too.
class Archive
{
  std::shared_ptr<void> lib;
  archive* a;

public:
  Archive(FILE* file)
  {
    a = archive_read_new();
    lib = std::shared_ptr<archive>(a, archive_read_free);
    archive_read_support_format_7zip(a);
    archive_read_support_format_gnutar(a);
    archive_read_support_format_rar(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);
    if (archive_read_open_FILE(a, file))
    {
      throw std::runtime_error("Error reading archive");
    }
  }

  void extractTo(const std::string& path)
  {
    for (;;)
    {
      archive_entry* entry{};
      auto r = archive_read_next_header(a, &entry);
      if (r == ARCHIVE_EOF)
        break;
      if (r != ARCHIVE_OK)
        throw std::runtime_error("Error reading archive");
      std::string fileName = archive_entry_pathname(entry);
      if (!extension_format_detail::isSafeArchiveEntryPath(fileName))
        throw std::runtime_error(
            "Archive entry escapes destination directory: " + fileName);

      auto fileType = archive_entry_filetype(entry);
      if (fileType == AE_IFLNK || fileType == AE_IFSOCK ||
          archive_entry_hardlink(entry))
      {
        // Symlinks/hardlinks/sockets could point outside the destination
        // directory; skip them entirely without extracting their data.
        archive_read_data_skip(a);
        continue;
      }

      auto fullPath = base::fix_path_separators(
          path + base::path_separator + fileName);

      if (fileType == AE_IFDIR)
      {
        base::make_all_directories(fullPath);
        continue;
      }

      base::make_all_directories(base::get_file_path(fullPath));
      auto out = base::open_file_with_exception(fullPath, "wb");
      for (;;)
      {
        const void* buff{};
        size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
        int64_t offset;
#else
        off_t offset;
#endif
        r = archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;
        if (r != ARCHIVE_OK)
          throw std::runtime_error("Error reading archive");
        fwrite(buff, size, 1, out.get());
      }
    }
  }
};

} // namespace app

#endif
