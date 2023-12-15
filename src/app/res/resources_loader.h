// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/task_manager.h"
#include "app/res/resource.h"
#include "base/injection.h"

#include <atomic>
#include <memory>

namespace app {

  class ResourcesLoader : public Injectable<ResourcesLoader> {
  public:
    using Callback = std::function<void(Resource)>;
    void load(Callback&& callback);

    virtual std::vector<std::string> resourcesLocation() const = 0;

  protected:
    virtual Resource loadResource(const std::string& fileName) = 0;

  private:
    TaskMonitor task;
  };

} // namespace app
