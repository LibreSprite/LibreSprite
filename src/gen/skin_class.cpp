// Aseprite Code Generator
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/skin_class.h"

#include "base/string.h"
#include "gen/common.h"

#include <iostream>
#include <vector>

void gen_skin_class(TiXmlDocument* doc, const std::string& inputFn)
{
  std::vector<std::string> dimensions;
  std::vector<std::string> colors;
  std::vector<std::string> parts;
  std::vector<std::string> styles;

  TiXmlHandle handle(doc);
  TiXmlElement* elem = handle
    .FirstChild("skin")
    .FirstChild("dimensions")
    .FirstChild("dim").ToElement();
  while (elem) {
    const char* id = elem->Attribute("id");
    dimensions.push_back(id);
    elem = elem->NextSiblingElement();
  }

  elem = handle
    .FirstChild("skin")
    .FirstChild("colors")
    .FirstChild("color").ToElement();
  while (elem) {
    const char* id = elem->Attribute("id");
    colors.push_back(id);
    elem = elem->NextSiblingElement();
  }

  elem = handle
    .FirstChild("skin")
    .FirstChild("parts")
    .FirstChild("part").ToElement();
  while (elem) {
    const char* id = elem->Attribute("id");
    if (!strchr(id, ':'))
      parts.push_back(id);
    elem = elem->NextSiblingElement();
  }

  elem = handle
    .FirstChild("skin")
    .FirstChild("stylesheet")
    .FirstChild("style").ToElement();
  while (elem) {
    const char* id = elem->Attribute("id");
    if (!strchr(id, ':'))
      styles.push_back(id);
    elem = elem->NextSiblingElement();
  }

  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n"
    << "#pragma once\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n"
    << "\n"
    << "  template<typename T>\n"
    << "  class SkinFile {\n"
    << "  public:\n"
    << "\n";

  // Dimensions sub class
  std::cout
    << "    class Dimensions {\n"
    << "      template<typename> friend class SkinFile;\n"
    << "    public:\n";
  for (auto dimension : dimensions) {
    std::string id = convert_xmlid_to_cppid(dimension, false);
    std::cout
      << "      int " << id << "() const { return m_" << id << "; }\n";
  }
  std::cout
    << "    private:\n";
  for (auto dimension : dimensions) {
    std::string id = convert_xmlid_to_cppid(dimension, false);
    std::cout
      << "      int m_" << id << ";\n";
  }
  std::cout
    << "    };\n";

  // Colors sub class
  std::cout
    << "    class Colors {\n"
    << "      template<typename> friend class SkinFile;\n"
    << "    public:\n";
  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout
      << "      gfx::Color " << id << "() const { return m_" << id << "; }\n";
  }
  std::cout
    << "    private:\n";
  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout
      << "      gfx::Color m_" << id << ";\n";
  }
  std::cout
    << "    };\n";

  // Parts sub class
  std::cout
    << "    class Parts {\n"
    << "      template<typename> friend class SkinFile;\n"
    << "    public:\n";
  for (auto part : parts) {
    std::string id = convert_xmlid_to_cppid(part, false);
    std::cout
      << "      const skin::SkinPartPtr& " << id << "() const { return m_" << id << "; }\n";
  }
  std::cout
    << "    private:\n";
  for (auto part : parts) {
    std::string id = convert_xmlid_to_cppid(part, false);
    std::cout
      << "      skin::SkinPartPtr m_" << id << ";\n";
  }
  std::cout
    << "    };\n";

  // Styles sub class
  std::cout
    << "\n"
    << "    class Styles {\n"
    << "      template<typename> friend class SkinFile;\n"
    << "    public:\n";
  for (auto style : styles) {
    std::string id = convert_xmlid_to_cppid(style, false);
    std::cout
      << "      skin::Style* " << id << "() const { return m_" << id << "; }\n";
  }
  std::cout
    << "    private:\n";
  for (auto style : styles) {
    std::string id = convert_xmlid_to_cppid(style, false);
    std::cout
      << "      skin::Style* m_" << id << ";\n";
  }
  std::cout
    << "    };\n";

  std::cout
    << "\n"
    << "    Dimensions dimensions;\n"
    << "    Colors colors;\n"
    << "    Parts parts;\n"
    << "    Styles styles;\n"
    << "\n"
    << "  protected:\n"
    << "    void updateInternals() {\n";
  for (auto dimension : dimensions) {
    std::string id = convert_xmlid_to_cppid(dimension, false);
    std::cout << "      byId(dimensions.m_" << id
              << ", \"" << dimension << "\");\n";
  }
  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout << "      byId(colors.m_" << id
              << ", \"" << color << "\");\n";
  }
  for (auto part : parts) {
    std::string id = convert_xmlid_to_cppid(part, false);
    std::cout << "      byId(parts.m_" << id
              << ", \"" << part << "\");\n";
  }
  for (auto style : styles) {
    std::string id = convert_xmlid_to_cppid(style, false);
    std::cout << "      byId(styles.m_" << id
              << ", \"" << style << "\");\n";
  }
  std::cout
    << "    }\n"
    << "\n"
    << "  private:\n"
    << "    void byId(int& dimension, const std::string& id) {\n"
    << "      dimension = static_cast<T*>(this)->getDimensionById(id);\n"
    << "    }\n"
    << "    void byId(gfx::Color& color, const std::string& id) {\n"
    << "      color = static_cast<T*>(this)->getColorById(id);\n"
    << "    }\n"
    << "    void byId(skin::SkinPartPtr& part, const std::string& id) {\n"
    << "      part = static_cast<T*>(this)->getPartById(id);\n"
    << "    }\n"
    << "    void byId(skin::Style*& style, const std::string& id) {\n"
    << "      style = static_cast<T*>(this)->getStyle(id);\n"
    << "    }\n";

  std::cout
    << "  };\n"
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n";
}
