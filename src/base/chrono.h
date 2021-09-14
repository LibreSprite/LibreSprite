// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace base {

  class Chrono {
  public:
    Chrono();
    ~Chrono();
    void reset();
    double elapsed() const;

  private:
    class ChronoImpl;
    ChronoImpl* m_impl;
  };

} // namespace base
