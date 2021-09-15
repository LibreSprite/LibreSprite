// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/command.h"

#include <string>

namespace app {
  class FileSelectorDelegate;

  class SaveFileBaseCommand : public Command {
  public:
    SaveFileBaseCommand(const char* shortName, const char* friendlyName, CommandFlags flags);

    std::string selectedFilename() const {
      return m_selectedFilename;
    }

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;

    bool saveAsDialog(Context* context, const char* dlgTitle,
                      FileSelectorDelegate* delegate = nullptr);

    std::string m_filename;
    std::string m_filenameFormat;
    std::string m_selectedFilename;
  };

} // namespace app
