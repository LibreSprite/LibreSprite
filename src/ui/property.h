// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"

#include <string>
#include <memory>

namespace ui {

  class Property {
  public:
    Property(const std::string& name);
    virtual ~Property();

    std::string getName() const;

  private:
    std::string m_name;

    DISABLE_COPYING(Property);
  };

  typedef std::shared_ptr<Property> PropertyPtr;

} // namespace ui
