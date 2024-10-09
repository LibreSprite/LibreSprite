// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/algorithm/hline.h"
#include "gfx/fwd.h"

namespace doc {
  namespace algorithm {

    void polygon(int vertices, const int* points, int pointStride, void* data, AlgoHLine proc);

    template<typename Container, typename Func>
    void polygon(const Container& container, Func&& func) {
      polygon(container.size(), (const int*) &container[0], sizeof(container[0])/sizeof(int), &func,
              [](int x, int y, int x2, void* ptr){
                (*reinterpret_cast<Func*>(ptr))(x, y, x2);
              });
    }

  }
}
