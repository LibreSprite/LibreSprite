// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace app {

  // A ruler inside the editor. It is used by SelectBoxState to show
  // rulers that can be dragged by the user.
  class Ruler {
  public:
    enum Orientation { Horizontal, Vertical };

    Ruler()
      : m_orientation(Horizontal)
      , m_position(0)
    {
    }

    Ruler(Orientation orientation, int position)
      : m_orientation(orientation)
      , m_position(position)
    {
    }

    Orientation getOrientation() const
    {
      return m_orientation;
    }

    int getPosition() const
    {
      return m_position;
    }

    void setPosition(int position)
    {
      m_position = position;
    }

  private:
    Orientation m_orientation;
    int m_position;
  };

} // namespace app
