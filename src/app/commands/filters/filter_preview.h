// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "ui/timer.h"
#include "ui/widget.h"

namespace app {

  class FilterManagerImpl;

  // Invisible widget to control a effect-preview in the current editor.
  class FilterPreview : public ui::Widget {
  public:
    FilterPreview(FilterManagerImpl* filterMgr);
    ~FilterPreview();

    void stop();
    void restartPreview();
    FilterManagerImpl* getFilterManager() const;

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:
    FilterManagerImpl* m_filterMgr;
    inject<ui::Timer> m_timer = ui::Timer::create(1, *this);
  };

} // namespace app
