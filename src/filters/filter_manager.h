// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "filters/target.h"

namespace doc {
  class Image;
}

namespace filters {

  class FilterIndexedData;

  // Information given to a filter (Filter interface) to apply it to a
  // single row. Basically an Filter implementation has to obtain
  // colors from getSourceAddress(), applies some kind of transformation
  // to that color, and save the result in getDestinationAddress().
  // This process must be repeated getWidth() times.
  class FilterManager {
  public:
    virtual ~FilterManager() { }

    // Gets the address of the first pixel which has the original color
    // to apply the filter.
    virtual const void* getSourceAddress() = 0;

    // Gets the address of the first pixel which is destination of the
    // filter.
    virtual void* getDestinationAddress() = 0;

    // Returns the width of the row to apply the filter. You must apply
    // the Filter "getWidth()" times, in each pixel from getSourceAddress().
    virtual int getWidth() = 0;

    // Returns the target of the Filter, i.e. what channels/components
    // (e.g. Red, Green, or Blue) will be modified by the filter.
    virtual Target getTarget() = 0;

    // Returns a interface needed by filters which operate over indexed
    // images. FilterIndexedData interface provides a Palette and a
    // RgbMap to help the filter to make its job.
    virtual FilterIndexedData* getIndexedData() = 0;

    // Returns true if you should skip the current pixel (do not apply
    // the filter). You must increment all your internal source and
    // destination address pointers one pixel without applying the
    // filter.
    //
    // This method is used to skip non-selected pixels (when the
    // selection is actived).
    virtual bool skipPixel() = 0;

    //////////////////////////////////////////////////////////////////////
    // Special members for 2D filters like convolution matrices.

    // Returns the source image.
    virtual const doc::Image* getSourceImage() = 0;

    // Returns the first X coordinate of the row to apply the filter.
    virtual int x() = 0;

    // Returns the Y coordinate of the row.
    virtual int y() = 0;

  };

} // namespace filters
