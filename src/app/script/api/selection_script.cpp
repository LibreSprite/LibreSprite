// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "app/app.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/color.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/tools/script_tool_driver.h"
#include "app/tools/script_tool_loop.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "doc/layer_index.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
  app::Document* activeDocument() {
    auto* doc = app::UIContext::instance()->activeDocument();
    if (!doc)
      throw std::runtime_error{"No active document"};
    return doc;
  }

  // Parse the points array (an array of [x, y] in sprite space, as in
  // tools.stroke's points, without the pressure value - the selection
  // tools ignore it).
  std::vector<gfx::Point> parsePoints(JSON::Value& points, const char* methodName)
  {
    if (!points.isArray())
      throw std::runtime_error(std::string(methodName) +
        ": 'points' must be an array of [x, y]");
    auto& arr = points.array();
    if (arr.empty())
      throw std::runtime_error(std::string(methodName) + ": at least one point is needed");
    std::vector<gfx::Point> pts;
    for (size_t i = 0; i < arr.size(); ++i) {
      auto& p = arr[i];
      if (!p.isArray() || p.array().size() < 2 ||
          !p.array()[0].isNumber() || !p.array()[1].isNumber())
        throw std::runtime_error(std::string(methodName) + ": each point must be [x, y]");
      pts.push_back(gfx::Point((int)p.array()[0].number(), (int)p.array()[1].number()));
    }
    return pts;
  }

  // The selection mode: "replace" (default), "add" or "subtract" (the
  // ToolLoopModifiers the selection ink reads during the gesture).
  app::tools::ToolLoopModifiers parseSelectionMode(JSON::Value& opts, const char* methodName)
  {
    auto it = opts.object().find("mode");
    if (it == opts.object().end() || !it->second.isString())
      return app::tools::ToolLoopModifiers::kReplaceSelection;
    const std::string mode = it->second.string();
    if (mode == "replace")
      return app::tools::ToolLoopModifiers::kReplaceSelection;
    if (mode == "add")
      return app::tools::ToolLoopModifiers::kAddSelection;
    if (mode == "subtract")
      return app::tools::ToolLoopModifiers::kSubtractSelection;
    throw std::runtime_error(std::string(methodName) + ": unknown mode '" + mode +
      "' (\"replace\", \"add\" or \"subtract\")");
  }

  // The target shared by the tool-based selection methods: the active
  // document, the frame (default: the active frame) and the GUI colors
  // (the selection ink only uses them for the preview, but the loop
  // config requires valid colors). needLayer is the floodfill point
  // shape (the magic wand), which reads the source layer's pixels; the
  // other selection tools take no layer - the mask is document-wide
  // and the GUI's create_tool_loop passes a nullptr layer for the
  // non-floodfill selection inks.
  struct SelectionTarget {
    app::UIContext* context = nullptr;
    app::Document* document = nullptr;
    doc::Sprite* sprite = nullptr;
    doc::frame_t frame = 0;
    doc::Layer* layer = nullptr;
    app::Color fg;
    app::Color bg;
  };

  SelectionTarget parseSelectionTarget(JSON::Value& opts, const char* methodName, bool needLayer)
  {
    auto& o = opts.object();

    app::UIContext* ctx = app::UIContext::instance();
    if (!ctx)
      throw std::runtime_error(std::string(methodName) + ": no UI context is available");

    app::Document* document = ctx->activeDocument();
    if (!document)
      throw std::runtime_error(std::string(methodName) + ": no active document");
    doc::Sprite* sprite = document->sprite();
    if (!sprite)
      throw std::runtime_error(std::string(methodName) + ": the active document has no sprite");

    SelectionTarget target;
    target.context = ctx;
    target.document = document;
    target.sprite = sprite;

    // Frame (default: the active frame)
    int frame = 0;
    auto frameIt = o.find("frame");
    if (frameIt != o.end() && frameIt->second.isNumber())
      frame = (int)frameIt->second.number();
    else if (app::current_editor)
      frame = app::current_editor->getSite().frame();
    if (frame < 0 || frame >= sprite->totalFrames())
      throw std::runtime_error(std::string(methodName) + ": the frame is out of range");
    target.frame = (doc::frame_t)frame;

    // Layer (the wand's floodfill source; default: the active layer)
    if (needLayer) {
      int layerIndex = 0;
      auto layerIt = o.find("layer");
      if (layerIt != o.end() && layerIt->second.isNumber())
        layerIndex = (int)layerIt->second.number();
      else if (app::current_editor)
        layerIndex = (int)app::current_editor->getSite().layerIndex();
      doc::Layer* layer = sprite->indexToLayer(doc::LayerIndex(layerIndex));
      if (!layer)
        throw std::runtime_error(std::string(methodName) + ": the layer index is out of range");
      target.layer = layer;
    }

    // Colors (default: the GUI fg/bg)
    app::ColorBar* colorBar = app::ColorBar::instance();
    if (!colorBar)
      throw std::runtime_error(std::string(methodName) + ": the color bar is unavailable in this mode");
    app::Color fg = colorBar->getFgColor();
    app::Color bg = colorBar->getBgColor();
    if (!fg.isValid())
      throw std::runtime_error(std::string(methodName) + ": the foreground color is invalid");
    if (!bg.isValid())
      throw std::runtime_error(std::string(methodName) + ": the background color is invalid");
    target.fg = fg;
    target.bg = bg;

    return target;
  }

  // Build the loop config for a selection tool (the fixed tool id, the
  // GUI brush and colors, the selection mode).
  app::ScriptToolLoopConfig buildSelectionConfig(const char* toolId, JSON::Value& opts,
                                                 const char* methodName, bool needLayer)
  {
    // The options are optional (a missing argument is padded with an
    // undefined value); anything else is an error. Check before any
    // .object() call converts the value.
    if (!opts.isUndefined() && !opts.isNull() && !opts.isObject())
      throw std::runtime_error(std::string(methodName) + ": the options must be an object");

    SelectionTarget target = parseSelectionTarget(opts, methodName, needLayer);

    app::ScriptToolLoopConfig cfg;
    cfg.context = target.context;
    cfg.document = target.document;
    cfg.tool = app::resolve_tool(toolId);
    cfg.frame = target.frame;
    cfg.layer = target.layer;
    cfg.fg = target.fg;
    cfg.bg = target.bg;

    app::ContextBar* contextBar = app::App::instance()->contextBar();
    if (!contextBar)
      throw std::runtime_error(std::string(methodName) + ": the context bar is unavailable in this mode");
    cfg.brush = contextBar->activeBrush(cfg.tool);
    if (!cfg.brush)
      throw std::runtime_error(std::string(methodName) + ": no active brush is available");

    cfg.modifiers = parseSelectionMode(opts, methodName);
    return cfg;
  }
} // namespace

class SelectionExtension : public Extension {
public:
  SelectionExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, SelectionSite>("Selection");
    clazz.setConstructor() = []() -> std::shared_ptr<SelectionSite> {
      static std::shared_ptr<SelectionSite> site = std::make_shared<SelectionSite>();
      return site;
    };

    clazz.addMethod("select") = [](SelectionSite&, double x, double y, double w, double h) -> JSON::Value {
      auto* doc = activeDocument();
      doc::Mask newMask;
      if (w > 0 && h > 0)
        newMask.replace(gfx::Rect((int)x, (int)y, (int)w, (int)h));
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetMask(doc, &newMask));
      tx.commit();
      return {};
    };

    clazz.addMethod("selectAll") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      doc::Mask newMask;
      newMask.replace(doc->sprite()->bounds());
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetMask(doc, &newMask));
      tx.commit();
      return {};
    };

    clazz.addMethod("deselect") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::DeselectMask(doc));
      tx.commit();
      return {};
    };

    clazz.addGetter("bounds") = [](SelectionSite&) -> JSON::Value {
      auto* doc = activeDocument();
      if (!doc->isMaskVisible())
        return JSON::Value{JSON::Special::Null};
      gfx::Rect b = doc->mask()->bounds();
      return JSON::makeObject({
        {"x", (double)b.x},
        {"y", (double)b.y},
        {"width", (double)b.w},
        {"height", (double)b.h},
      });
    };

    // lasso(points, { mode, frame }) runs the lasso tool: the freehand
    // loop through the points selects the enclosed region (contour and
    // fill, like the GUI tool). One undo step; inside an
    // app.transaction() scope the call joins the scope's transaction.
    clazz.addMethod("lasso") = [](SelectionSite&, JSON::Value& points, JSON::Value& opts) -> JSON::Value {
      std::vector<gfx::Point> pts = parsePoints(points, "selection.lasso");
      app::ScriptToolLoopConfig cfg = buildSelectionConfig("lasso", opts, "selection.lasso", false);
      app::run_tool_events(cfg, app::freehand(pts));
      return {};
    };

    // polygon(points, { mode, frame }) runs the polygonal lasso tool:
    // the polygon through the points selects its interior (contour and
    // fill, like the GUI tool; the gesture ends with the repeated-point
    // finish of the driver's point_by_point sequence).
    clazz.addMethod("polygon") = [](SelectionSite&, JSON::Value& points, JSON::Value& opts) -> JSON::Value {
      std::vector<gfx::Point> pts = parsePoints(points, "selection.polygon");
      if (pts.size() < 3)
        throw std::runtime_error("selection.polygon: at least three points are needed");
      app::ScriptToolLoopConfig cfg = buildSelectionConfig("polygonal_lasso", opts, "selection.polygon", false);
      app::run_tool_events(cfg, app::point_by_point(pts));
      return {};
    };

    // magicWand(x, y, { mode, tolerance, contiguous, layer, frame })
    // runs the magic wand tool: selects the pixels of the clicked color
    // (flood fill, like the GUI tool; tolerance/contiguous override the
    // tool preferences, as tools.fill).
    clazz.addMethod("magicWand") = [](SelectionSite&, double x, double y, JSON::Value& opts) -> JSON::Value {
      app::ScriptToolLoopConfig cfg = buildSelectionConfig("magic_wand", opts, "selection.magicWand", true);

      // Tolerance / contiguous (default: the tool preferences)
      app::ToolPreferences& toolPref = app::Preferences::instance().tool(cfg.tool);
      int tolerance = toolPref.tolerance();
      auto tolIt = opts.object().find("tolerance");
      if (tolIt != opts.object().end() && tolIt->second.isNumber()) {
        tolerance = (int)tolIt->second.number();
        if (tolerance < 0 || tolerance > 255)
          throw std::runtime_error("selection.magicWand: the tolerance must be between 0 and 255");
      }
      cfg.tolerance = tolerance;
      bool contiguous = toolPref.contiguous();
      auto contIt = opts.object().find("contiguous");
      if (contIt != opts.object().end() && contIt->second.isBoolean())
        contiguous = contIt->second.boolean();
      cfg.contiguous = contiguous;

      app::run_tool_events(cfg, app::one_point(gfx::Point((int)x, (int)y)));
      return {};
    };
  }
};

static di::provide<Extension, SelectionExtension> x{"selection"};
