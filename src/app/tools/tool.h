// Aseprite    - Copyright (C) 2001-2015  David Capello
// LibreSprite - Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/tools/fill.h"
#include "app/tools/trace_policy.h"

#include <memory>
#include <string>

namespace app {
  namespace tools {

    class Controller;
    class Ink;
    class Intertwine;
    class PointShape;
    class ToolGroup;

    // A drawing tool
    class Tool {
    public:

      Tool(ToolGroup* group,
           const std::string& id,
           const std::string& text,
           const std::string& tips,
           int default_brush_size)
        : m_group(group)
        , m_id(id)
        , m_text(text)
        , m_tips(tips)
        , m_default_brush_size(default_brush_size)
      { }

      virtual ~Tool()
      { }

      const ToolGroup* getGroup() const { return m_group; }
      const std::string& getId() const { return m_id; }
      const std::string& getText() const { return m_text; }
      const std::string& getTips() const { return m_tips; }
      int getDefaultBrushSize() const { return m_default_brush_size; }

      Fill getFill(int button) { return m_button[button].m_fill; }
      std::shared_ptr<Ink> getInk(int button) { return m_button[button].m_ink; }
      Controller* getController(int button) { return m_button[button].m_controller; }
      PointShape* getPointShape(int button) { return m_button[button].m_point_shape; }
      Intertwine* getIntertwine(int button) { return m_button[button].m_intertwine; }
      TracePolicy getTracePolicy(int button) { return m_button[button].m_trace_policy; }

      void setFill(int button, Fill fill) { m_button[button].m_fill = fill; }
      void setInk(int button, std::shared_ptr<Ink> ink) { m_button[button].m_ink = ink; }
      void setController(int button, Controller* controller) { m_button[button].m_controller = controller; }
      void setPointShape(int button, PointShape* point_shape) { m_button[button].m_point_shape = point_shape; }
      void setIntertwine(int button, Intertwine* intertwine) { m_button[button].m_intertwine = intertwine; }
      void setTracePolicy(int button, TracePolicy trace_policy) { m_button[button].m_trace_policy = trace_policy; }

    private:
      ToolGroup* m_group;
      std::string m_id;
      std::string m_text;
      std::string m_tips;
      int m_default_brush_size;

      struct {
        Fill m_fill;
        std::shared_ptr<Ink> m_ink;
        Controller* m_controller;
        PointShape* m_point_shape;
        Intertwine* m_intertwine;
        TracePolicy m_trace_policy;
      } m_button[2]; // Two buttons: [0] left and [1] right

    };

  } // namespace tools
} // namespace app
