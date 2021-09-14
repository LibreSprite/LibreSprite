// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/crash/session.h"
#include "base/disable_copying.h"

#include <vector>

namespace doc {
  class Context;
}

namespace app {
namespace crash {
  class BackupObserver;

  class DataRecovery {
  public:
    typedef std::vector<SessionPtr> Sessions;

    DataRecovery(doc::Context* context);
    ~DataRecovery();

    // Returns the list of sessions that can be recovered.
    const Sessions& sessions() { return m_sessions; }

  private:
    Sessions m_sessions;
    SessionPtr m_inProgress;
    BackupObserver* m_backup;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace crash
} // namespace app
