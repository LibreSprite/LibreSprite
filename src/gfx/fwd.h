// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace gfx {

template<typename T> class BorderT;
template<typename T> class PointT;
template<typename T> class RectT;
template<typename T> class SizeT;

typedef BorderT<int> Border;
typedef PointT<int> Point;
typedef RectT<int> Rect;
typedef SizeT<int> Size;

class Region;

} // namespace gfx
