// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/resources_loader_delegate.h"
#include "app/res/resources_loader.h"
#include "app/res/resource.h"

#include "app/file/palette_file.h"
#include "app/file_system.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/scoped_value.h"
#include "doc/palette.h"

using namespace app;

class PalettesLoaderDelegate : public ResourcesLoader {
public:
  std::string resourcesLocation() const override {
    std::string path;
    ResourceFinder rf;
    rf.includeDataDir("palettes");
    while (rf.next()) {
      if (base::is_directory(rf.filename())) {
        path = rf.filename();
        break;
      }
    }
    return base::fix_path_separators(path);
  }

  Resource loadResource(const std::string& filename) override {
    return {
      load_palette(filename.c_str()),
      base::get_file_title(filename)
    };
  }
};

static ResourcesLoader::Regular<PalettesLoaderDelegate> palRL("palette");
