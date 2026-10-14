// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script_permission_dialog.h"

#include "app/script/script_permissions.h"
#include "base/path.h"

namespace app {

  ScriptPermissionDialog::ScriptPermissionDialog(const std::string& scriptPath,
                                                 const std::set<std::string>& missing) {
    // The script name (a Label is fixed at construction, so it is added
    // to the placeholder box like the other custom widgets).
    scriptNamePlaceholder()->addChild(new ui::Label(base::get_file_name(scriptPath)));

    // Show (and default-check) only the not-yet-granted permissions.
    permFsRead()->setVisible(missing.count(app::ScriptPermissions::kFsRead));
    permFsWrite()->setVisible(missing.count(app::ScriptPermissions::kFsWrite));
    permFetch()->setVisible(missing.count(app::ScriptPermissions::kFetch));
    permAppLaunch()->setVisible(missing.count(app::ScriptPermissions::kAppLaunch));
    permFsRead()->setSelected(missing.count(app::ScriptPermissions::kFsRead));
    permFsWrite()->setSelected(missing.count(app::ScriptPermissions::kFsWrite));
    permFetch()->setSelected(missing.count(app::ScriptPermissions::kFetch));
    permAppLaunch()->setSelected(missing.count(app::ScriptPermissions::kAppLaunch));
  }

  bool ScriptPermissionDialog::show(const std::string& scriptPath,
                                    const std::set<std::string>& missing,
                                    std::set<std::string>& outGranted) {
    ScriptPermissionDialog dialog(scriptPath, missing);
    dialog.openWindowInForeground();
    if (dialog.closer() != dialog.ok())
      return false;

    outGranted.clear();
    if (dialog.permFsRead()->isSelected())
      outGranted.insert(app::ScriptPermissions::kFsRead);
    if (dialog.permFsWrite()->isSelected())
      outGranted.insert(app::ScriptPermissions::kFsWrite);
    if (dialog.permFetch()->isSelected())
      outGranted.insert(app::ScriptPermissions::kFetch);
    if (dialog.permAppLaunch()->isSelected())
      outGranted.insert(app::ScriptPermissions::kAppLaunch);
    return true;
  }

} // namespace app
