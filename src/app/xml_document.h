// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/exception.h"

#include "tinyxml.h"

#include <memory>
#include <string>

namespace app {

  typedef std::shared_ptr<TiXmlDocument> XmlDocumentRef;

  XmlDocumentRef open_xml(const std::string& filename);
  void save_xml(XmlDocumentRef doc, const std::string& filename);

  bool bool_attr_is_true(const TiXmlElement* elem, const char* attrName);

} // namespace app
