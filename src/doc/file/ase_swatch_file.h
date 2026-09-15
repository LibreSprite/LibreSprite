// Aseprite Document Library
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <memory>

namespace doc {

  class Palette;

  namespace file {

    // Loads an Adobe Swatch Exchange (.ase) palette file. The .ase extension is
    // shared with Aseprite sprites, so this returns nullptr if the file is not
    // an "ASEF" swatch (letting the caller fall back to the sprite loader).
    std::shared_ptr<Palette> load_ase_swatch_file(const char* filename);

  } // namespace file
} // namespace doc
