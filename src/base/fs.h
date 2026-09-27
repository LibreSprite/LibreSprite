// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>
#include <vector>

namespace base {

  class Time;

  bool is_file(const std::string& path);
  bool is_directory(const std::string& path);

  size_t file_size(const std::string& path);

  void move_file(const std::string& src, const std::string& dst);
  void copy_file(const std::string& src, const std::string& dst);
  void delete_file(const std::string& path);

  bool has_readonly_attr(const std::string& path);
  void remove_readonly_attr(const std::string& path);

  Time get_modification_time(const std::string& path);

  void make_directory(const std::string& path);
  void make_all_directories(const std::string& path);
  void remove_directory(const std::string& path);

  std::string get_current_path();
  std::string get_app_path();
  std::string get_temp_path();
  std::string get_user_docs_folder();
  std::vector<std::string> get_font_paths();
#if __APPLE__
  std::string get_lib_app_support_path();
#endif

  // If the given filename is a relative path, it converts the
  // filename to an absolute one.
  std::string get_canonical_path(const std::string& path);

  std::vector<std::string> list_files(const std::string& path);

  // Rejects a relative path component that would walk back out of the
  // directory it's meant to be joined onto (a ".." component that
  // survives normalization), or that carries an embedded NUL a
  // C-string handoff would silently truncate. A leading '/' is not by
  // itself rejected: callers join this onto a base directory by plain
  // string concatenation rather than std::filesystem::path::append(),
  // so it can never be reinterpreted as an absolute path - it just
  // becomes a contained subdirectory.
  bool has_path_traversal(const std::string& relativePath);

  // Rejects absolute paths (Unix, Windows drive-letter, or UNC) and any
  // path containing a ".." component, so an archive entry (or any other
  // externally supplied relative path) can never write outside the
  // directory it's being extracted/joined into (zip-slip). Also rejects
  // any ':' (not just at the drive-letter position) and Windows reserved
  // device basenames, since on Windows a colon anywhere in a filename
  // addresses an NTFS Alternate Data Stream of the base file rather than
  // a normal file, and CON/NUL/AUX/COM1-9/LPT1-9 are special device
  // names regardless of extension (see issue #219). These checks are
  // applied on every platform, since a crafted archive is not
  // necessarily extracted on the platform it targets.
  bool is_safe_archive_entry_path(const std::string& fileName);

} // namespace base
