// LibreSprite | Copyright (C) 2023       LibreSprite contributors
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

#include <archive.h>
#include <archive_entry.h>

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

class Archive {
  std::shared_ptr<void> lib;
  archive* a;
public:
  Archive(FILE* file) {
    a = archive_read_new();
    lib = std::shared_ptr<archive>(a, archive_read_free);
    archive_read_support_format_7zip(a);
    archive_read_support_format_gnutar(a);
    archive_read_support_format_rar(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);
    if (archive_read_open_FILE(a, file)) {
      throw std::runtime_error("Error reading archive");
    }
  }

  void extractTo(const std::string& path) {
    for (;;) {
      archive_entry* entry{};
      auto r = archive_read_next_header(a, &entry);
      if (r == ARCHIVE_EOF)
        break;
      if (r != ARCHIVE_OK)
        throw std::runtime_error("Error reading archive");
      std::string fileName = archive_entry_pathname(entry);
      bool isDir = archive_entry_filetype(entry) == AE_IFDIR;
      auto out = open_file_with_exception(path + base::path_separator + fileName, "wb");
      for (;;) {
        const void *buff{};
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
        if (isDir)
          continue;
        fwrite(buff, size, 1, out.get());
      }
    }
  }
};

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
    base::make_all_directories(themePath);
    archive.extractTo(themePath);
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
