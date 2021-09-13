// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_cel.h"

#include "doc/cel.h"

namespace app {
namespace cmd {

using namespace doc;

WithCel::WithCel(std::shared_ptr<Cel> cel)
  : m_celId(cel->id())
{
}

std::shared_ptr<Cel> WithCel::cel()
{
  return std::static_pointer_cast<Cel>(get<Cel>(m_celId)->shared_from_this());
}

} // namespace cmd
} // namespace app
