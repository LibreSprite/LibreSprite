// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "obs/signal.h"

namespace base {

template<typename R>
using Signal0 = obs::signal<R()>;

template<typename R, typename A1>
using Signal1 = obs::signal<R(A1)>;

template<typename R, typename A1, typename A2>
using Signal2 = obs::signal<R(A1, A2)>;

} // namespace base
