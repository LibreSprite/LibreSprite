// Aseprite    - Copyright (C) 2001-2015  David Capello
// LibreSprite - Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>

#include "app/tools/tool.h"

class TiXmlElement;

namespace app {
  namespace tools {

    namespace WellKnownTools {
      extern const char* RectangularMarquee;
      extern const char* Pencil;
      extern const char* Eraser;
      extern const char* Eyedropper;
      extern const char* Hand;
    };

    namespace WellKnownInks {
      extern const char* Selection;
      extern const char* Paint;
      extern const char* PaintFg;
      extern const char* PaintBg;
      extern const char* PaintCopy;
      extern const char* PaintLockAlpha;
      extern const char* Shading;
      extern const char* Eraser;
      extern const char* ReplaceFgWithBg;
      extern const char* ReplaceBgWithFg;
      extern const char* PickFg;
      extern const char* PickBg;
      extern const char* Zoom;
      extern const char* Scroll;
      extern const char* Move;
      extern const char* Slice;
      extern const char* Blur;
      extern const char* Jumble;
    };

    namespace WellKnownIntertwiners {
      extern const char* None;
      extern const char* AsLines;
      extern const char* AsRectangles;
      extern const char* AsEllipses;
      extern const char* AsBezier;
      extern const char* AsPixelPerfect;
    };

    namespace WellKnownPointShapes {
      extern const char* None;
      extern const char* Pixel;
      extern const char* Brush;
      extern const char* FloodFill;
      extern const char* Spray;
    };

    typedef std::list<Tool*> ToolList;
    typedef ToolList::iterator ToolIterator;
    typedef ToolList::const_iterator ToolConstIterator;

    typedef std::list<ToolGroup*> ToolGroupList;

    // Loads and maintains the group of tools specified in the gui.xml file
    class ToolBox {
    public:
      ToolBox();
      ~ToolBox();

      ToolGroupList::iterator begin_group() { return m_groups.begin(); }
      ToolGroupList::iterator end_group() { return m_groups.end(); }

      ToolIterator begin() { return m_tools.begin(); }
      ToolIterator end() { return m_tools.end(); }
      ToolConstIterator begin() const { return m_tools.begin(); }
      ToolConstIterator end() const { return m_tools.end(); }

      Tool* getToolById(const std::string& id);
      std::shared_ptr<Ink> getInkById(const std::string& id);
      Intertwine* getIntertwinerById(const std::string& id);
      PointShape* getPointShapeById(const std::string& id);
      int getGroupsCount() const { return m_groups.size(); }

    private:
      void loadTools();
      void loadToolProperties(TiXmlElement* xmlTool, Tool* tool, int button, const std::string& suffix);

      std::map<std::string, std::shared_ptr<Ink>> m_inks;
      std::map<std::string, Controller*> m_controllers;
      std::map<std::string, PointShape*> m_pointshapers;
      std::map<std::string, Intertwine*> m_intertwiners;

      ToolGroupList m_groups;
      ToolList m_tools;
    };

  } // namespace tools
} // namespace app
