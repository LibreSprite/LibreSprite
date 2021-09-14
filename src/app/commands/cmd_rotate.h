// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/command.h"

namespace app {

  class RotateCommand : public Command {
  public:
    RotateCommand();
    Command* clone() const override { return new RotateCommand(*this); }

    bool flipMask() const { return m_flipMask; }
    int angle() const { return m_angle; }

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
    std::string onGetFriendlyName() const override;

  private:
    bool m_flipMask;
    int m_angle;
  };

} // namespace app
