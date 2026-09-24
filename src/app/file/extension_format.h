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
#include "base/string.h"

#include <archive.h>
#include <archive_entry.h>

#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace app
{

namespace extension_format_detail
{

// Rejects absolute paths (Unix, Windows drive-letter, or UNC) and any path
// containing a ".." component, so an archive entry can never write outside
// the destination directory it's being extracted to (zip-slip). Also
// rejects any ':' (not just at the drive-letter position) and Windows
// reserved device basenames, since on Windows a colon anywhere in a
// filename addresses an NTFS Alternate Data Stream of the base file
// rather than a normal file, and CON/NUL/AUX/COM1-9/LPT1-9 are special
// device names regardless of extension (see issue #219).
inline bool isSafeArchiveEntryPath(const std::string& fileName)
{
  if (fileName.empty())
    return false;
  if (fileName[0] == '/' || fileName[0] == '\\')
    return false;
  if (fileName.find(':') != std::string::npos)
    return false;

  static const std::string kReservedNames[] = {
      "con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4",
      "com5", "com6", "com7", "com8", "com9", "lpt1", "lpt2", "lpt3",
      "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};

  // Windows has historically also treated certain Unicode superscript
  // digits as equivalent to the corresponding ASCII digit in COM/LPT
  // device names (e.g. "COM¹" behaves like "COM1").
  static const std::string kReservedSuperscriptDigits[] = {
      "\xC2\xB9",     // U+00B9 SUPERSCRIPT ONE   -> 1
      "\xC2\xB2",     // U+00B2 SUPERSCRIPT TWO   -> 2
      "\xC2\xB3",     // U+00B3 SUPERSCRIPT THREE -> 3
      "\xE2\x81\xB4", // U+2074 SUPERSCRIPT FOUR
      "\xE2\x81\xB5", // U+2075 SUPERSCRIPT FIVE
      "\xE2\x81\xB6", // U+2076 SUPERSCRIPT SIX
      "\xE2\x81\xB7", // U+2077 SUPERSCRIPT SEVEN
      "\xE2\x81\xB8", // U+2078 SUPERSCRIPT EIGHT
      "\xE2\x81\xB9", // U+2079 SUPERSCRIPT NINE
  };

  size_t start = 0;
  while (start <= fileName.size())
  {
    size_t end = fileName.find_first_of("/\\", start);
    if (end == std::string::npos)
      end = fileName.size();
    std::string component = fileName.substr(start, end - start);
    if (component == "..")
      return false;

    // Windows strips trailing spaces and dots off a path component
    // before resolving it, so "con " and "con." are treated exactly
    // like "con" for the reserved-device-name check below.
    while (!component.empty() &&
           (component.back() == ' ' || component.back() == '.'))
      component.pop_back();

    // Compare the component up to a trailing extension too (Windows
    // treats "NUL.txt" the same as "NUL").
    std::string baseName =
        base::string_to_lower(component.substr(0, component.find('.')));
    for (auto& reserved : kReservedNames)
    {
      if (baseName == reserved)
        return false;
    }
    for (auto& digit : kReservedSuperscriptDigits)
    {
      if (baseName == "com" + digit || baseName == "lpt" + digit)
        return false;
    }
    start = end + 1;
  }
  return true;
}

// Creates a fresh directory with an unpredictable name as a sibling of
// parentDir's own eventual contents (so a later move_file() onto a path
// under parentDir stays on the same filesystem and is atomic), and
// returns its path. Used to extract an archive into before moving it
// into place: since the name is unguessable and freshly created by us,
// nothing else can have planted a symlink inside it beforehand, closing
// the TOCTOU window a plain mkdir(destination)+extract would leave open
// between destination's creation and the first write into it.
inline std::string makeStagingDirectory(const std::string& parentDir)
{
  std::random_device rd;
  std::mt19937_64 rng(rd());
  std::runtime_error lastError("Could not create a staging directory");
  for (int attempt = 0; attempt < 8; ++attempt)
  {
    std::ostringstream name;
    name << ".besprited-extract-" << std::hex << rng() << rng();
    auto path = base::fix_path_separators(
        parentDir + base::path_separator + name.str());
    try
    {
      base::make_directory(path);
      return path;
    }
    catch (const std::exception& ex)
    {
      // Name collision (astronomically unlikely) or a transient
      // failure; retry with a fresh random name.
      lastError = std::runtime_error(ex.what());
    }
  }
  throw lastError;
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
    // Compressed archive formats let a tiny entry expand to an arbitrary
    // amount of data on disk (a "zip bomb", CWE-409). Cap the total bytes
    // written across the whole archive (see issue #219).
    constexpr uint64_t kMaxExtractedBytes = 1ull << 30; // 1 GiB
    // A byte cap alone doesn't stop an archive with millions of
    // zero-byte entries from exhausting inodes/directory entries.
    constexpr uint64_t kMaxEntries = 100000;
    uint64_t totalWritten = 0;
    uint64_t entryCount = 0;

    for (;;)
    {
      archive_entry* entry{};
      auto r = archive_read_next_header(a, &entry);
      if (r == ARCHIVE_EOF)
        break;
      if (r != ARCHIVE_OK)
        throw std::runtime_error("Error reading archive");
      if (++entryCount > kMaxEntries)
        throw std::runtime_error("Archive contains too many entries");
      const char* rawName = archive_entry_pathname(entry);
      if (!rawName)
        throw std::runtime_error("Archive entry has no name");
      std::string fileName = rawName;
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
        totalWritten += size;
        if (totalWritten > kMaxExtractedBytes)
          throw std::runtime_error("Archive expands past the size limit");
        fwrite(buff, size, 1, out.get());
      }
    }
  }
};

} // namespace app

#endif
