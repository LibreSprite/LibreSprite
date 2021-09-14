// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/command.h"
#include "doc/algorithm/flip_type.h"

namespace app {

  class FlipCommand : public Command {
  public:
    FlipCommand();
    Command* clone() const override { return new FlipCommand(*this); }

    doc::algorithm::FlipType getFlipType() const { return m_flipType; }

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
    std::string onGetFriendlyName() const override;

  private:
    bool m_flipMask;
    doc::algorithm::FlipType m_flipType;
  };

} // namespace app
