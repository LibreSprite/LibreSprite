// LibreSprite
// Copyright (c) 2024 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once
#include <iterator>

namespace base {
  template <typename iterator>
  class range {
    std::pair<iterator, iterator> pair;
  public:
    typedef typename iterator::iterator_category iterator_category;
    typedef typename iterator::value_type value_type;
    typedef typename iterator::difference_type difference_type;
    typedef typename iterator::pointer pointer;
    typedef typename iterator::reference reference;

    range(const std::pair<iterator, iterator>& pair) : pair{pair} {}

    iterator begin() {return pair.first;}

    iterator end() {return pair.second;}

    bool empty() {return begin() == end();}
  };
}
