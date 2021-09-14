// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/observable.h"
#include "doc/color_mode.h"
#include "doc/object_id.h"
#include "doc/sprite.h"
#include "doc/sprites_observer.h"

#include <vector>

namespace doc {
  class Document;

  class Sprites : base::Observable<SpritesObserver> {
  public:
    typedef std::vector<Sprite*>::iterator iterator;
    typedef std::vector<Sprite*>::const_iterator const_iterator;

    Sprites(Document* doc);
    ~Sprites();

    iterator begin() { return m_sprites.begin(); }
    iterator end() { return m_sprites.end(); }
    const_iterator begin() const { return m_sprites.begin(); }
    const_iterator end() const { return m_sprites.end(); }

    Sprite* front() const { return m_sprites.front(); }
    Sprite* back() const { return m_sprites.back(); }

    int size() const { return (int)m_sprites.size(); }
    bool empty() const { return m_sprites.empty(); }

    Sprite* add(int width, int height, ColorMode mode = ColorMode::RGB, int ncolors = 256);
    Sprite* add(Sprite* spr);
    void remove(Sprite* spr);
    void move(Sprite* spr, int index);

    Sprite* operator[](int index) const { return m_sprites[index]; }

  private:
    // Deletes all sprites in the list (calling "delete" operation).
    void deleteAll();

    Document* m_doc;
    std::vector<Sprite*> m_sprites;

    DISABLE_COPYING(Sprites);
  };

} // namespace doc
