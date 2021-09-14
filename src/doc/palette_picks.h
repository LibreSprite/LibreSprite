// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <algorithm>
#include <vector>

namespace doc {

  class PalettePicks {
  public:
    typedef std::vector<bool> list_type;
    typedef list_type::iterator iterator;
    typedef list_type::const_iterator const_iterator;
    typedef list_type::reference reference;
    typedef list_type::const_reference const_reference;

    PalettePicks() { }
    PalettePicks(int n) : m_items(n, false) { }

    int size() const { return int(m_items.size()); }
    int picks() const { return std::count(m_items.begin(), m_items.end(), true); }

    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }

    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }

    const_reference operator[](int idx) const { return m_items[idx]; }
    reference operator[](int idx) { return m_items[idx]; }

    void resize(int n) {
      m_items.resize(n, false);
    }

    void clear() {
      std::fill(m_items.begin(), m_items.end(), false);
    }

    void all() {
      std::fill(m_items.begin(), m_items.end(), true);
    }

    // If there is just one selected color (or none), we select them all.
    void pickAllIfNeeded() {
      if (picks() < 2)
        all();
    }

    int firstPick() const {
      for (int i=0; i<size(); ++i)
        if (m_items[i])
          return i;
      return -1;
    }

    int lastPick() const {
      for (int i=size()-1; i>=0; --i)
        if (m_items[i])
          return i;
      return -1;
    }

  private:
    list_type m_items;
  };

} // namespace doc
