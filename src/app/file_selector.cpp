// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file_selector.h"

#include "app/app.h"
#include "app/pref/preferences.h"
#include "app/ui/file_selector.h"
#include "base/split_string.h"
#include "she/display.h"
#include "she/system.h"

namespace app {

std::string show_file_selector(
  const std::string& title,
  const std::string& initialPath,
  const std::string& showExtensions,
  FileSelectorType type,
  FileSelectorDelegate* delegate)
{
  FileSelector fileSelector(type, delegate);
  return fileSelector.show(title, initialPath, showExtensions);
}

} // namespace app
