// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "ui/view.h"

namespace app {

  class FileListView : public ui::View {
  public:
    FileListView() { }

  private:
    void onScrollRegion(ui::ScrollRegionEvent& ev);
  };

} // namespace app
