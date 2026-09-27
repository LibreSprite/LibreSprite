// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fs.h"
#include "base/split_string.h"
#include "base/string.h"

#include <filesystem>

#ifdef _WIN32
  #include "base/fs_win32.h"
#else
  #include "base/fs_unix.h"
#endif

namespace base {

bool has_path_traversal(const std::string& relativePath)
{
  if (relativePath.empty())
    return false;

  // A JS (or other embedded-language) string can carry an embedded NUL
  // that std::string preserves but a later C-string handoff truncates -
  // the two would see different strings, so reject outright rather than
  // trying to reason about what's checked vs. what's actually opened.
  if (relativePath.find('\0') != std::string::npos)
    return true;

  namespace fs = std::filesystem;

  // Prefixing with "./" keeps a leading '/' (or a Windows drive letter)
  // from being parsed as an absolute path/root-name by
  // std::filesystem::path - it becomes just another path component,
  // matching how callers actually use this string (string
  // concatenation onto a base directory, never path::append()).
  fs::path normalized = fs::path("./" + relativePath).lexically_normal();
  auto it = normalized.begin();
  return it != normalized.end() && *it == "..";
}

bool is_safe_archive_entry_path(const std::string& fileName)
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

void make_all_directories(const std::string& path)
{
  std::vector<std::string> parts;
  split_string(path, parts, "/\\");

  std::string intermediate;
  for (const std::string& component : parts) {
    if (component.empty()) {
      if (intermediate.empty())
        intermediate += "/";
      continue;
    }

    intermediate = join_path(intermediate, component);

    if (is_file(intermediate))
      throw std::runtime_error("Error creating directory (a component is a file name)");
    else if (!is_directory(intermediate))
      make_directory(intermediate);
  }
}

} // namespace base
