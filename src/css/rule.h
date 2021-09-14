// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "css/map.h"

#include <map>
#include <string>

namespace css {

  class Rule {
  public:
    Rule() { }
    Rule(const std::string& name);

    const std::string& name() const { return m_name; }

  private:
    std::string m_name;
  };

  typedef Map<Rule*> Rules;

} // namespace css
