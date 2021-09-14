// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace app {

  // Handles available to transform a region of pixels in the editor.
  enum HandleType {
    // No handle selected
    NoHandle,
    // This is the handle to move the pixels region, generally, the
    // whole region activates this handle.
    MoveHandle,
    // One of the region's corders to scale.
    ScaleNWHandle, ScaleNHandle, ScaleNEHandle,
    ScaleWHandle,                ScaleEHandle,
    ScaleSWHandle, ScaleSHandle, ScaleSEHandle,
    // One of the region's corders to rotate.
    RotateNWHandle, RotateNHandle, RotateNEHandle,
    RotateWHandle,                 RotateEHandle,
    RotateSWHandle, RotateSHandle, RotateSEHandle,
    // Handle used to move the pivot
    PivotHandle,
  };

} // namespace app
