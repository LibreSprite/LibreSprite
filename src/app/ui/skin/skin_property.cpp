// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include <memory>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_property.h"

#include "ui/widget.h"

namespace app {
namespace skin {

const char* SkinProperty::Name = "SkinProperty";

SkinProperty::SkinProperty() : Property(Name) {
  m_look = NormalLook;
  m_miniFont = false;
  m_upperLeft = 0;
  m_upperRight = 0;
  m_lowerLeft = 0;
  m_lowerRight = 0;
}

SkinPropertyPtr get_skin_property(ui::Widget* widget) {
  auto skinProp = std::static_pointer_cast<SkinProperty>(widget->getProperty(SkinProperty::Name));
  if (!skinProp) {
    skinProp.reset(new SkinProperty);
    widget->setProperty(skinProp);
  }

  return skinProp;
}

} // namespace skin
} // namespace app
