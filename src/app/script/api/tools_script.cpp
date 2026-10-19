// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// The `tools` object: run a GUI drawing-tool gesture (freehand tools,
// line, rectangle, ellipse, curve, paint bucket) with synthetic pointer
// events. Each method is one tool gesture, so it is one undo step (the
// driver owns the Transaction; inside an app.transaction() scope the
// gestures join the scope's transaction instead). The target document
// becomes active after the call.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "app/app.h"
#include "app/color.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/script_tool_driver.h"
#include "app/tools/script_tool_loop.h"
#include "app/tools/tool.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/brush.h"
#include "doc/brush_pattern.h"
#include "doc/brush_type.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_index.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "gfx/point.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  // The freehand tools that stroke() supports (see data/gui.xml).
  bool is_supported_tool(const std::string& id)
  {
    return id == "pencil" || id == "eraser" || id == "spray" ||
           id == "contour" || id == "blur" || id == "jumble";
  }

  // Extract a doc::Image from a JSON::Value (a JS `Image` object). The
  // native wrapper stores the base type, so the pointer is checked with a
  // dynamic_cast through the polymorphic doc::Object base.
  doc::Image* imageFromValue(JSON::Value& v, const char* methodName)
  {
    if (!v.isNative())
      throw std::runtime_error(std::string(methodName) + ": the brush 'image' must be an Image object");
    auto& native = v.native();
    auto* image = dynamic_cast<doc::Image*>(static_cast<doc::Object*>(native.first.get()));
    if (!image)
      throw std::runtime_error(std::string(methodName) + ": the brush 'image' must be an Image object");
    return image;
  }

  // Build the brush: a copy of the GUI's active brush for the tool, with the
  // given fields applied.
  doc::BrushRef parseBrush(JSON::Value& opts, app::tools::Tool* tool, const char* methodName)
  {
    app::ContextBar* contextBar = app::App::instance()->contextBar();
    if (!contextBar)
      throw std::runtime_error(std::string(methodName) + ": the context bar is unavailable in this mode");
    doc::BrushRef active = contextBar->activeBrush(tool);
    if (!active)
      throw std::runtime_error(std::string(methodName) + ": no active brush is available");

    // Copy the active brush so the overrides don't mutate the GUI state.
    doc::BrushRef brush(new doc::Brush(*active));

    auto it = opts.object().find("brush");
    if (it == opts.object().end() || !it->second.isObject())
      return brush;

    auto& obj = it->second.object();

    auto typeIt = obj.find("type");
    if (typeIt != obj.end() && typeIt->second.isString()) {
      // string_id_to_brush_type falls back to circle for unknown ids, so
      // validate explicitly.
      const std::string type = typeIt->second.string();
      if (type == "circle")      brush->setType(doc::kCircleBrushType);
      else if (type == "square") brush->setType(doc::kSquareBrushType);
      else if (type == "line")   brush->setType(doc::kLineBrushType);
      else if (type == "image")  brush->setType(doc::kImageBrushType);
      else
        throw std::runtime_error(std::string(methodName) + ": unknown brush type: " + type);
    }

    // size / angle only apply to the shape brushes
    if (brush->type() != doc::kImageBrushType) {
      auto sizeIt = obj.find("size");
      if (sizeIt != obj.end() && sizeIt->second.isNumber()) {
        int size = (int)sizeIt->second.number();
        if (size < doc::Brush::kMinBrushSize || size > doc::Brush::kMaxBrushSize)
          throw std::runtime_error(std::string(methodName) + ": the brush size must be between " +
            std::to_string(doc::Brush::kMinBrushSize) + " and " +
            std::to_string(doc::Brush::kMaxBrushSize));
        brush->setSize(size);
      }

      auto angleIt = obj.find("angle");
      if (angleIt != obj.end() && angleIt->second.isNumber()) {
        int angle = (int)angleIt->second.number();
        if (angle < 0 || angle > 359)
          throw std::runtime_error(std::string(methodName) + ": the brush angle must be between 0 and 359");
        brush->setAngle(angle);
      }
    }

    auto patternIt = obj.find("pattern");
    if (patternIt != obj.end() && patternIt->second.isString()) {
      const std::string pattern = patternIt->second.string();
      if (pattern == "aligned_to_source")
        brush->setPattern(doc::BrushPattern::ALIGNED_TO_SRC);
      else if (pattern == "aligned_to_destination")
        brush->setPattern(doc::BrushPattern::ALIGNED_TO_DST);
      else if (pattern == "paint_brush")
        brush->setPattern(doc::BrushPattern::PAINT_BRUSH);
      else
        throw std::runtime_error(std::string(methodName) + ": unknown brush pattern: " + pattern);
    }

    auto imageIt = obj.find("image");
    if (imageIt != obj.end() && imageIt->second.isNative())
      brush->setImage(imageFromValue(imageIt->second, methodName));

    // An image brush must have an image to stamp.
    if (brush->type() == doc::kImageBrushType && !brush->image())
      throw std::runtime_error(std::string(methodName) + ": an image brush needs the 'image' field");

    return brush;
  }

  // Parse the options shared by all tools.* methods (tool, ink, brush,
  // color, opacity, layer, frame) and fill the config that
  // run_tool_events consumes. `defaultToolId` is the tool used when the
  // `tool` option is absent; when it is null the GUI's active tool is
  // used (stroke). Throws std::runtime_error on an invalid value.
  app::ScriptToolLoopConfig parseToolOptions(JSON::Value& opts, const char* methodName, const char* defaultToolId)
  {
    auto& o = opts.object();

    app::UIContext* ctx = app::UIContext::instance();
    if (!ctx)
      throw std::runtime_error(std::string(methodName) + ": no UI context is available");

    // Tool (default: `defaultToolId`, or the GUI's active tool when null)
    app::tools::Tool* tool;
    auto toolIt = o.find("tool");
    if (toolIt != o.end() && toolIt->second.isString())
      tool = app::resolve_tool(toolIt->second.string());
    else if (defaultToolId)
      tool = app::resolve_tool(defaultToolId);
    else {
      tool = app::App::instance()->activeTool();
      if (!tool)
        throw std::runtime_error(std::string(methodName) + ": no active tool is available");
    }

    // Document
    app::Document* document = ctx->activeDocument();
    if (!document)
      throw std::runtime_error(std::string(methodName) + ": no active document");
    doc::Sprite* sprite = document->sprite();
    if (!sprite)
      throw std::runtime_error(std::string(methodName) + ": the active document has no sprite");

    app::ScriptToolLoopConfig cfg;
    cfg.context = ctx;
    cfg.document = document;
    cfg.tool = tool;

    // Frame (default: the active frame)
    int frame = 0;
    auto frameIt = o.find("frame");
    if (frameIt != o.end() && frameIt->second.isNumber())
      frame = (int)frameIt->second.number();
    else if (app::current_editor)
      frame = app::current_editor->getSite().frame();
    if (frame < 0 || frame >= sprite->totalFrames())
      throw std::runtime_error(std::string(methodName) + ": the frame is out of range");
    cfg.frame = (doc::frame_t)frame;

    // Layer (default: the active layer)
    int layerIndex = 0;
    auto layerIt = o.find("layer");
    if (layerIt != o.end() && layerIt->second.isNumber())
      layerIndex = (int)layerIt->second.number();
    else if (app::current_editor)
      layerIndex = (int)app::current_editor->getSite().layerIndex();
    doc::Layer* layer = sprite->indexToLayer(doc::LayerIndex(layerIndex));
    if (!layer)
      throw std::runtime_error(std::string(methodName) + ": the layer index is out of range");
    cfg.layer = layer;

    // Ink (default: the tool's ink)
    auto inkIt = o.find("ink");
    if (inkIt != o.end() && inkIt->second.isString())
      cfg.inkOverride = app::resolve_ink(inkIt->second.string());

    // Brush (default: the GUI's active brush for the tool)
    cfg.brush = parseBrush(opts, tool, methodName);

    // Colors (default: the GUI fg/bg; 'color' overrides the fg)
    app::ColorBar* colorBar = app::ColorBar::instance();
    if (!colorBar)
      throw std::runtime_error(std::string(methodName) + ": the color bar is unavailable in this mode");
    app::Color fg = colorBar->getFgColor();
    app::Color bg = colorBar->getBgColor();
    auto colorIt = o.find("color");
    if (colorIt != o.end() && colorIt->second.isNumber()) {
      uint32_t c = (uint32_t)colorIt->second.number();
      fg = app::Color::fromRgb(doc::rgba_getr(c), doc::rgba_getg(c),
                               doc::rgba_getb(c), doc::rgba_geta(c));
    }
    if (!fg.isValid())
      throw std::runtime_error(std::string(methodName) + ": the foreground color is invalid");
    if (!bg.isValid())
      throw std::runtime_error(std::string(methodName) + ": the background color is invalid");
    cfg.fg = fg;
    cfg.bg = bg;

    // Opacity (default: the tool preference)
    int opacity = app::Preferences::instance().tool(tool).opacity();
    auto opacityIt = o.find("opacity");
    if (opacityIt != o.end() && opacityIt->second.isNumber()) {
      opacity = (int)opacityIt->second.number();
      if (opacity < 0 || opacity > 255)
        throw std::runtime_error(std::string(methodName) + ": the opacity must be between 0 and 255");
    }
    cfg.opacity = opacity;

    return cfg;
  }

  // Parse a required integer coordinate (sprite space; negative values are
  // allowed - the pipeline clips like the GUI).
  int parseCoord(JSON::Value& opts, const char* key, const char* methodName)
  {
    auto it = opts.object().find(key);
    if (it == opts.object().end() || !it->second.isNumber())
      throw std::runtime_error(std::string(methodName) + ": '" + key + "' is required (a sprite-space integer)");
    return (int)it->second.number();
  }

  // Parse the optional `filled` boolean, or `def` when absent.
  bool parseFilled(JSON::Value& opts, bool def)
  {
    auto it = opts.object().find("filled");
    if (it == opts.object().end() || !it->second.isBoolean())
      return def;
    return it->second.boolean();
  }

} // anonymous namespace

class ToolsObject {
public:
};

class ToolsExtension : public Extension {
public:
  ToolsExtension() {
    auto& cls = addClass<void, ToolsObject>("Tools");
    cls.setConstructor() = []() -> std::shared_ptr<ToolsObject> {
      return std::make_shared<ToolsObject>();
    };

    // stroke(options) runs a freehand tool gesture with synthetic pointer
    // events; the driver owns the Transaction, so one call is one undo step.
    cls.addMethod("stroke") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.stroke: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.stroke", nullptr);

      if (!is_supported_tool(cfg.tool->getId()))
        throw std::runtime_error("tools.stroke: '" + cfg.tool->getId() +
          "' is not a supported freehand tool (pencil, eraser, spray, "
          "contour, blur, jumble)");

      // Filled (default: the tool preference)
      cfg.filled = parseFilled(opts, app::Preferences::instance().tool(cfg.tool).filled());

      // Points (required): an array of [x, y] or [x, y, pressure]
      auto pointsIt = opts.object().find("points");
      if (pointsIt == opts.object().end() || !pointsIt->second.isArray())
        throw std::runtime_error("tools.stroke: 'points' is required "
          "(an array of [x, y] or [x, y, pressure])");
      auto& pointsArr = pointsIt->second.array();
      if (pointsArr.empty())
        throw std::runtime_error("tools.stroke: at least one point is needed");

      // Build the gesture: one press, one move per extra point, one release
      // (the driver's freehand() sequence, with per-point pressure).
      std::vector<app::ScriptPointerEvent> events;
      for (size_t i = 0; i < pointsArr.size(); ++i) {
        auto& p = pointsArr[i];
        if (!p.isArray())
          throw std::runtime_error("tools.stroke: each point must be [x, y] or [x, y, pressure]");
        auto& pa = p.array();
        if (pa.size() < 2)
          throw std::runtime_error("tools.stroke: each point needs at least x and y");
        float pressure = 1.0f;
        if (pa.size() >= 3) {
          pressure = (float)pa[2].number();
          if (pressure < 0.0f || pressure > 1.0f)
            throw std::runtime_error("tools.stroke: the pressure must be between 0 and 1");
        }
        app::ScriptPointerEvent ev;
        ev.kind = (i == 0 ? app::ScriptPointerEvent::Down: app::ScriptPointerEvent::Move);
        ev.point = gfx::Point((int)pa[0].number(), (int)pa[1].number());
        ev.pressure = pressure;
        events.push_back(ev);
      }
      app::ScriptPointerEvent up;
      up.kind = app::ScriptPointerEvent::Up;
      up.point = events.back().point;
      up.pressure = events.back().pressure;
      events.push_back(up);

      // Performance: a 1000-point freehand stroke takes ~45 ms headless
      // (Release build). Each pointer step calls
      // notifySpritePixelsInvalidated, the same per-event cost the GUI pays,
      // so long strokes are not batched. Revisit batching if scripts need
      // very long freehand paths.
      //
      // Run the gesture. The driver makes the document active and
      // validates the layer state. It owns the Transaction (one undo
      // step) unless an app.transaction() scope is open: then the loop
      // joins the scope's transaction (the driver sets
      // cfg.externalTransaction).
      app::run_tool_events(cfg, events);
      return {};
    };

    // line({x1, y1, x2, y2, ...}) runs the line tool (two_points gesture).
    cls.addMethod("line") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.line: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.line", "line");
      gfx::Point p1(parseCoord(opts, "x1", "tools.line"), parseCoord(opts, "y1", "tools.line"));
      gfx::Point p2(parseCoord(opts, "x2", "tools.line"), parseCoord(opts, "y2", "tools.line"));
      app::run_tool_events(cfg, app::two_points(p1, p2));
      return {};
    };

    // rectangle({x1, y1, x2, y2, filled, ...}) runs the rectangle tool
    // (two_points gesture); filled picks outline vs solid.
    cls.addMethod("rectangle") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.rectangle: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.rectangle", "rectangle");
      gfx::Point p1(parseCoord(opts, "x1", "tools.rectangle"), parseCoord(opts, "y1", "tools.rectangle"));
      gfx::Point p2(parseCoord(opts, "x2", "tools.rectangle"), parseCoord(opts, "y2", "tools.rectangle"));
      cfg.filled = parseFilled(opts, false);
      app::run_tool_events(cfg, app::two_points(p1, p2));
      return {};
    };

    // ellipse({x1, y1, x2, y2, filled, ...}) runs the ellipse tool
    // (two_points gesture); filled picks outline vs solid.
    cls.addMethod("ellipse") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.ellipse: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.ellipse", "ellipse");
      gfx::Point p1(parseCoord(opts, "x1", "tools.ellipse"), parseCoord(opts, "y1", "tools.ellipse"));
      gfx::Point p2(parseCoord(opts, "x2", "tools.ellipse"), parseCoord(opts, "y2", "tools.ellipse"));
      cfg.filled = parseFilled(opts, false);
      app::run_tool_events(cfg, app::two_points(p1, p2));
      return {};
    };

    // curve({x1, y1, x2, y2, x3, y3, x4, y4, ...}) runs the curve tool
    // (four_points gesture): a cubic bezier from (x1,y1) to (x4,y4) with
    // control points (x2,y2) and (x3,y3).
    cls.addMethod("curve") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.curve: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.curve", "curve");
      gfx::Point p1(parseCoord(opts, "x1", "tools.curve"), parseCoord(opts, "y1", "tools.curve"));
      gfx::Point p2(parseCoord(opts, "x2", "tools.curve"), parseCoord(opts, "y2", "tools.curve"));
      gfx::Point p3(parseCoord(opts, "x3", "tools.curve"), parseCoord(opts, "y3", "tools.curve"));
      gfx::Point p4(parseCoord(opts, "x4", "tools.curve"), parseCoord(opts, "y4", "tools.curve"));
      app::run_tool_events(cfg, app::four_points(p1, p2, p3, p4));
      return {};
    };

    // fill({x, y, tolerance, contiguous, ...}) runs the paint bucket tool
    // (one_point gesture); tolerance/contiguous default to the tool prefs.
    cls.addMethod("fill") = [](ToolsObject&, JSON::Value& opts) -> JSON::Value {
      if (!opts.isObject())
        throw std::runtime_error("tools.fill: the options must be an object");

      app::ScriptToolLoopConfig cfg = parseToolOptions(opts, "tools.fill", "paint_bucket");
      gfx::Point p(parseCoord(opts, "x", "tools.fill"), parseCoord(opts, "y", "tools.fill"));

      // Tolerance / contiguous (default: the tool preferences)
      app::ToolPreferences& toolPref = app::Preferences::instance().tool(cfg.tool);
      int tolerance = toolPref.tolerance();
      auto tolIt = opts.object().find("tolerance");
      if (tolIt != opts.object().end() && tolIt->second.isNumber()) {
        tolerance = (int)tolIt->second.number();
        if (tolerance < 0 || tolerance > 255)
          throw std::runtime_error("tools.fill: the tolerance must be between 0 and 255");
      }
      cfg.tolerance = tolerance;
      bool contiguous = toolPref.contiguous();
      auto contIt = opts.object().find("contiguous");
      if (contIt != opts.object().end() && contIt->second.isBoolean())
        contiguous = contIt->second.boolean();
      cfg.contiguous = contiguous;

      app::run_tool_events(cfg, app::one_point(p));
      return {};
    };
  }

  std::string init(const std::string& language, JSON::Value&) override {
    if (language != "js")
      return "";
    return "globalThis.tools = new Tools();";
  }
};

static di::provide<Extension, ToolsExtension> toolsExt{"tools"};
