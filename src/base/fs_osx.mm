// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Foundation/Foundation.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace base {

std::string get_lib_app_support_path()
{
  NSArray* dirs = NSSearchPathForDirectoriesInDomains(
    NSApplicationSupportDirectory, NSUserDomainMask, YES);
  if (dirs) {
    NSString* dir = [dirs firstObject];
    if (dir)
      return std::string([dir UTF8String]);
  }
  return std::string();
}

std::vector<std::string> get_font_paths()
{
    std::vector<std::string> dirs = {
        "/System/Library/Fonts/Supplemental",
        "/System/Library/Fonts",
        "/Library/Fonts"
    };
    // The user fonts dir (a literal "~" cannot be opened).
    const char* home = getenv("HOME");
    if (home)
        dirs.push_back(std::string(home) + "/Library/Fonts");
    return dirs;
}

} // namespace base
