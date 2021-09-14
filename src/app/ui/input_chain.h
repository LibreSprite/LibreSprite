// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/observable.h"

#include <vector>

namespace app {

  class Context;
  class InputChainElement;

  // The chain of objects (in order) that want to receive
  // input/commands from the user (e.g. ColorBar, Timeline, and
  // Workspace/DocumentView). When each of these elements receive the
  // user focus, they call InputChain::prioritize().
  class InputChain {
  public:
    void prioritize(InputChainElement* element);

    bool canCut(Context* ctx);
    bool canCopy(Context* ctx);
    bool canPaste(Context* ctx);
    bool canClear(Context* ctx);

    void cut(Context* ctx);
    void copy(Context* ctx);
    void paste(Context* ctx);
    void clear(Context* ctx);
    void cancel(Context* ctx);

  private:
    std::vector<InputChainElement*> m_elements;
  };

} // namespace app
