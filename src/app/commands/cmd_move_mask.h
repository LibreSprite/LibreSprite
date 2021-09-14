// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/command.h"

namespace app {

  class MoveMaskCommand : public Command {
  public:
    enum Target { Boundaries, Content };
    enum Direction { Left, Up, Right, Down, }; // TODO join this enum with scroll command
    enum Units {
      Pixel,
      TileWidth,
      TileHeight,
      ZoomedPixel,
      ZoomedTileWidth,
      ZoomedTileHeight,
      ViewportWidth,
      ViewportHeight
    };

    MoveMaskCommand();
    Command* clone() const override { return new MoveMaskCommand(*this); }

    Target getTarget() const { return m_target; }
    gfx::Point getDelta(Context* context) const;

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
    std::string onGetFriendlyName() const override;

  private:
    Target m_target;
    Direction m_direction;
    Units m_units;
    int m_quantity;
    bool m_wrap;
  };

} // namespace app
