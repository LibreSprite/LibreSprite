// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"
#include "app/commands/command.h"

namespace doc {
  class Site;
}

namespace app {

  class EyedropperCommand : public Command {
  public:
    EyedropperCommand();
    Command* clone() const override { return new EyedropperCommand(*this); }

    // Returns the color in the given sprite pos.
    void pickSample(const doc::Site& site,
                    const gfx::Point& pixelPos,
                    app::Color& color);

  protected:
    void onLoadParams(const Params& params) override;
    void onExecute(Context* context) override;

    // True means "pick background color", false the foreground color.
    bool m_background;
  };

} // namespace app
