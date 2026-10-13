// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <set>
#include <string>

#include "script_permissions.xml.h"

namespace app {

  // Modal dialog shown the first time a script requests permissions:
  // lists the not-yet-granted ones (checked by default) and lets the
  // user pick which to grant.
  class ScriptPermissionDialog : public app::gen::ScriptPermissions {
  public:
    // Shows the dialog; returns false if the user cancelled (the script
    // must not run). outGranted receives the checked permissions.
    static bool show(const std::string& scriptPath,
                     const std::set<std::string>& missing,
                     std::set<std::string>& outGranted);

    ScriptPermissionDialog(const std::string& scriptPath,
                           const std::set<std::string>& missing);
  };

} // namespace app
