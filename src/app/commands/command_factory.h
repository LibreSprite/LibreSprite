// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace app {
  class Command;

  class CommandFactory {
  public:
#undef FOR_EACH_COMMAND
#define FOR_EACH_COMMAND(Name)                  \
    static Command* create##Name##Command();

#include "app/commands/commands_list.h"
#undef FOR_EACH_COMMAND
  };

} // namespace app
