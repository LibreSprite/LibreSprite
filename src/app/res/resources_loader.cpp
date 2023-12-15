// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/resources_loader.h"

#include "app/file_system.h"
#include "app/res/resource.h"
#include "app/res/resources_loader_delegate.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/scoped_value.h"

#include "app/task_manager.h"

namespace app {
  void ResourcesLoader::load(Callback&& callback) {
    auto fs = FileSystemModule::instance();
    LockFS lock{fs};
    std::size_t pending{};
    std::deque<std::string> files;

    for (auto& path : resourcesLocation()) {
        LOG("Loading resources from %s...\n", path.c_str());
        if (path.empty())
            return;

        IFileItem* item = fs->getFileItemFromPath(path);
        if (!item)
            continue;

        for (auto child : item->children()) {
            if (child->isFolder())
                continue;
            files.push_back(child->fileName());
            pending++;
        }
    }

    if (!pending) {
        callback({});
        return;
    }

    task = TaskManager::instance().addTask<Resource>(
      [this, files = std::move(files)](std::atomic_bool& isAlive) mutable -> Resource {
        if (!files.empty()) {
          auto next = files.front();
          files.pop_front();
          return loadResource(next);
        }
        isAlive = false;
        return {};
      },
      std::move(callback));
  }
} // namespace app
