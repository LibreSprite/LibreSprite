// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/sprite.h"

namespace doc {

  class SpritesObserver {
  public:
    virtual ~SpritesObserver() { }
    virtual void onAddSprite(Sprite* spr) { }
    virtual void onRemoveSprite(Sprite* spr) { }
  };

} // namespace doc
