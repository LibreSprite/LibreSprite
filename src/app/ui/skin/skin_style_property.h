// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/style.h"

#include <memory>

namespace app {
namespace skin {
  class Style;

  class SkinStyleProperty : public ui::Property {
  public:
    static const char* Name;

    SkinStyleProperty(Style* style);

    skin::Style* getStyle() const;

  private:
    skin::Style* m_style;
  };

  typedef std::shared_ptr<SkinStyleProperty> SkinStylePropertyPtr;

} // namespace skin
} // namespace app
