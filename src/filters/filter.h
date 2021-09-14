// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace filters {

  class FilterManager;

  // Interface which applies a filter to a sprite given a FilterManager
  // which indicates where we have to apply the filter.
  class Filter {
  public:
    virtual ~Filter() { }

    // Returns a proper name for the filter. It is used to show a label
    // with the Undo action.
    virtual const char* getName() = 0;

    // Applies the filter to one RGBA row. You must use
    // FilterManager::getSourceAddress() and advance 32 bits to modify
    // each pixel.
    virtual void applyToRgba(FilterManager* filterMgr) = 0;

    // Applies the filter to one grayscale row. You must use
    // FilterManager::getSourceAddress() and advance 16 bits to modify
    // each pixel.
    virtual void applyToGrayscale(FilterManager* filterMgr) = 0;

    // Applies the filter to one indexed row. You must use
    // FilterManager::getSourceAddress() and advance 8 bits to modify
    // each pixel.
    virtual void applyToIndexed(FilterManager* filterMgr) = 0;

  };

} // namespace filters
