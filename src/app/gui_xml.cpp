// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/gui_xml.h"

#include "app/resource_finder.h"
#include "base/fs.h"
#include "tinyxml2.h"

namespace app {

// static
GuiXml* GuiXml::instance()
{
  static GuiXml* singleton = 0;
  if (!singleton)
    singleton = new GuiXml();
  return singleton;
}

GuiXml::GuiXml()
{
  LOG("Loading gui.xml file...\n");

  ResourceFinder rf;
  rf.includeDataDir("gui.xml");
  if (!rf.findFirst())
    throw base::Exception("gui.xml was not found");

  // Load the XML file. As we've already checked "path" existence,
  // in a case of exception we should show the error and stop.
  m_doc = app::open_xml(rf.filename());
}

std::string GuiXml::version()
{
  tinyxml2::XMLHandle handle(m_doc.get());
  tinyxml2::XMLElement* xmlKey = handle.FirstChildElement("gui").ToElement();

  if (xmlKey && xmlKey->Attribute("version")) {
    const char* guixml_version = xmlKey->Attribute("version");
    return guixml_version;
  }
  else
    return "";
}

} // namespace app
