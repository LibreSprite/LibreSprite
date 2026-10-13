// LibreSprite | Copyright (C) 2023-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//

#if __has_include(<archive.h>)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/task_manager.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"
#include "ui/alert.h"

#include <filesystem>
#include <system_error>

// Included last: on Windows, <archive_entry.h> drags in <windows.h>, whose
// macros (TRANSPARENT, IMAGE_BITMAP, DIFFERENCE, ...) collide with
// identically-named enum members in the headers above.
#include "app/file/extension_format.h"

namespace app {

using namespace base;

class ExtensionFormat : public FileFormat {
  const char* onGetName() const override { return "aseprite-extension"; }
  const char* onGetExtensions() const override { return "aseprite-extension"; }
  int onGetFlags() const override {
    return FILE_SUPPORT_LOAD;
  }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override {return false;}
};

static FileFormat::Regular<ExtensionFormat> ff{"extension"};

bool ExtensionFormat::onLoad(FileOp* fop)
{
  if (fop->isOneFrame())
    return false;

  auto filename = fop->filename();
  FileHandle handle(open_file_with_exception(filename, "rb"));
  Archive archive{handle.get()};

  std::string skins;
  {
    ResourceFinder rf;
    rf.includeDataDir("skins");
    skins = rf.defaultFilename();
  }

  auto themeName = base::replace_extension(base::get_file_name(filename), "");
  themeName.pop_back();

  auto themePath = skins + base::path_separator + themeName;
  if (!base::is_directory(themePath)) {
    // Extract into a fresh staging directory and atomically move it into
    // place instead of extracting straight into themePath: a concurrent
    // local process could otherwise plant a symlink inside the
    // just-mkdir'd, still-empty themePath before extraction writes into it.
    base::make_all_directories(skins);
    auto stagingPath = extension_format_detail::makeStagingDirectory(skins);
    try {
      archive.extractTo(stagingPath);
      base::move_file(stagingPath, themePath);
    }
    catch (...) {
      std::error_code ec;
      std::filesystem::remove_all(stagingPath, ec);
      throw;
    }
  }

  if (themeName != Preferences::instance().theme.selected()) {
    TaskManager::instance().delayed([=]{
      Preferences::instance().theme.selected(themeName);
      ui::Alert::show(PACKAGE "<<You must restart the program to see the selected theme" "||&OK");
    });
  }

  return true;
}

} // namespace app

#endif
