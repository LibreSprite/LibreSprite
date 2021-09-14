// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/frame.h"
#include "doc/object_id.h"

#include <set>

namespace doc {
  class Cel;
  class Sprite;

  class CelsRange {
  public:
    enum Flags {
      ALL,
      UNIQUE,
    };

    CelsRange(const Sprite* sprite,
      frame_t first, frame_t last, Flags flags = ALL);

    class iterator {
    public:
      iterator();
      iterator(const Sprite* sprite, frame_t first, frame_t last, Flags flags);

      bool operator==(const iterator& other) const {
        return m_cel == other.m_cel;
      }

      bool operator!=(const iterator& other) const {
        return !operator==(other);
      }

      Cel* operator*() const {
        return m_cel;
      }

      iterator& operator++();

    private:
      Cel* m_cel;
      frame_t m_first, m_last;
      Flags m_flags;
      std::set<ObjectId> m_visited;
    };

    iterator begin() { return m_begin; }
    iterator end() { return m_end; }

  private:
    iterator m_begin, m_end;
  };

} // namespace doc
