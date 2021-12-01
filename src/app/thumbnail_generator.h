// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/mutex.h"

#include <memory>
#include <vector>

namespace base {
  class thread;
}

namespace app {
  class IFileItem;

  class ThumbnailGenerator {
  public:
    enum WorkerStatus { WithoutWorker, WorkingOnThumbnail, ThumbnailIsDone };

    static ThumbnailGenerator* instance();

    // Generate a thumbnail for the given file-item.  It must be called
    // from the GUI thread.
    void addWorkerToGenerateThumbnail(IFileItem* fileitem);

    // Returns the status of the worker that is generating the thumbnail
    // for the given file.
    WorkerStatus getWorkerStatus(IFileItem* fileitem, double& progress);

    // Checks the status of workers. If there are workers that already
    // done its job, we've to destroy them. This function must be called
    // from the GUI thread (because a thread is joint to it).
    // Returns true if there are workers generating thumbnails.
    bool checkWorkers();

    // Stops all workers generating thumbnails. This is an non-blocking
    // operation. The cancelation of all workers is done in a background
    // thread.
    void stopAllWorkers();

  private:
    void stopAllWorkersBackground();

    class Worker;
    typedef std::vector<Worker*> WorkerList;

    WorkerList m_workers;
    base::mutex m_workersAccess;
    std::unique_ptr<base::thread> m_stopThread;
  };
} // namespace app
