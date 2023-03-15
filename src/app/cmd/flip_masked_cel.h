// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd_sequence.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"
#include <memory>

namespace doc {
  class Cel;
}

namespace app {
namespace cmd {
  using namespace doc;

  class FlipMaskedCel : public CmdSequence {
  public:
    FlipMaskedCel(std::shared_ptr<Cel> cel, doc::algorithm::FlipType flipType);
  };

} // namespace cmd
} // namespace app
