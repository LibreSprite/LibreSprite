// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/script_tool_loop.h"

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tools/controller.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/symmetry.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/context_bar.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "render/render.h"

#include <stdexcept>

namespace app {

Context* ScriptToolLoop::validateConfig(const ScriptToolLoopConfig& config)
{
  if (!config.context || !config.document || !config.tool || !config.brush)
    throw std::runtime_error(
      "ScriptToolLoop: the context, document, tool and brush "
      "must be set in the config");

  if (config.context->activeDocument() != config.document)
    throw std::runtime_error(
      "ScriptToolLoop: the target document must be the active document "
      "(call UIContext::setActiveDocument before constructing the loop)");

  return config.context;
}

ScriptToolLoop::ScriptToolLoop(const ScriptToolLoopConfig& config)
  : m_context(ScriptToolLoop::validateConfig(config))
  , m_tool(config.tool)
  , m_brush(config.brush)
  , m_document(config.document)
  , m_sprite(config.document->sprite())
  , m_layer(config.layer)
  , m_frame(config.frame)
  , m_rgbMap(nullptr)
  , m_docPref(Preferences::instance().document(m_document))
  , m_opacity(config.opacity)
  , m_tolerance(config.tolerance)
  , m_contiguous(config.contiguous)
  , m_zoom(1, 1)
  , m_button(config.button)
  , m_ink((config.inkOverride ? config.inkOverride: m_tool->getInk(m_button))->clone())
  , m_controller(m_tool->getController(m_button))
  , m_pointShape(m_tool->getPointShape(m_button))
  , m_intertwine(m_tool->getIntertwine(m_button))
  , m_tracePolicy(m_tool->getTracePolicy(m_button))
  , m_symmetry(nullptr)
  , m_shadingRemap(nullptr)
  , m_colorTarget(m_layer ? ColorTarget(m_layer):
                            ColorTarget(ColorTarget::BackgroundLayer,
                                        m_sprite->pixelFormat(),
                                        m_sprite->transparentColor()))
  , m_fgColor(color_utils::color_for_target_mask(config.fg, m_colorTarget))
  , m_bgColor(color_utils::color_for_target_mask(config.bg, m_colorTarget))
  , m_primaryColor(m_button == tools::ToolLoop::Left ? m_fgColor: m_bgColor)
  , m_secondaryColor(m_button == tools::ToolLoop::Left ? m_bgColor: m_fgColor)
  , m_filled(false)
  , m_previewFilled(config.previewFilled)
  , m_sprayWidth(config.sprayWidth)
  , m_spraySpeed(config.spraySpeed)
  , m_useMask(false)
  , m_mask(nullptr)
  , m_canceled(false)
  , m_ownsTransaction(config.externalTransaction == nullptr)
  , m_ownedTransaction(config.externalTransaction ? nullptr:
                       new Transaction(
                         config.context,
                         m_tool->getText().c_str(),
                         ((getInk()->isSelection() ||
                           getInk()->isEyedropper() ||
                           getInk()->isScrollMovement() ||
                           getInk()->isSlice() ||
                           getInk()->isZoom()) ? DoesntModifyDocument:
                                                 ModifyDocument)))
  , m_transaction(config.externalTransaction ? *config.externalTransaction:
                                                *m_ownedTransaction)
  , m_floodfillSrcImage(nullptr)
{
  // Validate the layer and the colors (same checks as the GUI tool loop)
  if (m_layer) {
    if (!m_layer->isVisible())
      throw std::runtime_error("ScriptToolLoop: the layer is hidden");
    if (!m_layer->isEditable())
      throw std::runtime_error("ScriptToolLoop: the layer is locked");
  }
  else if (m_pointShape->isFloodFill())
    throw std::runtime_error("ScriptToolLoop: floodfill tools need a layer");

  if (!config.fg.isValid() || !config.bg.isValid())
    throw std::runtime_error("ScriptToolLoop: invalid fg or bg color");

  // Freehand-algorithm intertwine swap (same as the GUI tool loop)
  tools::FreehandAlgorithm algorithm = config.freehandAlgorithm;

  if (m_tracePolicy == tools::TracePolicy::Accumulate ||
      m_tracePolicy == tools::TracePolicy::AccumulateUpdateLast) {
    tools::ToolBox* toolbox = App::instance()->toolBox();

    switch (algorithm) {
      case tools::FreehandAlgorithm::PIXEL_PERFECT:
        // The GUI tool loop checks m_toolPref.brush.size() because the
        // context-bar brush is kept in sync with the preference; here
        // the brush comes from the caller, so check it directly.
        if (m_brush->size() == 1) {
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect);
          m_tracePolicy = tools::TracePolicy::AccumulateUpdateLast;
          break;
        }
        [[fallthrough]];
      case tools::FreehandAlgorithm::DEFAULT:
        m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines);
        m_tracePolicy = tools::TracePolicy::Accumulate;
        break;
      case tools::FreehandAlgorithm::DOTS:
        m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::None);
        m_tracePolicy = tools::TracePolicy::Accumulate;
        break;
    }
  }

  // Shading ink remap. The ContextBar exists in every script-capable
  // mode; fail clearly when the GUI is unavailable.
  if (m_ink->isShading()) {
    ContextBar* contextBar = App::instance()->contextBar();
    if (!contextBar)
      throw std::runtime_error(
        "ScriptToolLoop: the shading ink needs the context bar, "
        "which is unavailable in this mode");
    m_shadingRemap.reset(contextBar->createShadeRemap(
      m_button == tools::ToolLoop::Left));
  }

  // Prepare the floodfill source image (same as the GUI tool loop)
  if (m_pointShape->isFloodFill()) {
    if (config.floodFillReferTo == gen::FillReferTo::ALL_LAYERS) {
      m_floodfillSrcImage = Image::create(m_sprite->pixelFormat(),
                                          m_sprite->width(),
                                          m_sprite->height());
      m_floodfillSrcImage->clear(m_sprite->transparentColor());

      render::Render().renderSprite(
        m_floodfillSrcImage,
        m_sprite,
        m_frame,
        gfx::Clip(m_sprite->bounds()),
        render::Zoom(1, 1));
    }
    else {
      if (m_layer) {
        auto cel = m_layer->cel(m_frame);
        if (cel && (cel->x() != 0 || cel->y() != 0)) {
          m_floodfillSrcImage = Image::create(m_sprite->pixelFormat(),
                                              m_sprite->width(),
                                              m_sprite->height());
          m_floodfillSrcImage->clear(m_sprite->transparentColor());
          copy_image(m_floodfillSrcImage, cel->image(), cel->x(), cel->y());
        }
      }
    }
  }

  // Expand the cel canvas, with a Site built from the config
  doc::Site site;
  site.document(m_document);
  site.sprite(m_sprite);
  site.layer(m_layer);
  site.frame(m_frame);

  m_expandCelCanvas = std::make_unique<ExpandCelCanvas>(
    site,
    m_layer,
    filters::TiledMode::NONE,
    m_transaction,
    ExpandCelCanvas::Flags(
      ExpandCelCanvas::NeedsSource |
      // If the tool is freehand-like, we can use the modified
      // region directly as undo information to save the modified
      // pixels. See ExpandCelCanvas::commit() for details about this flag.
      (getController()->isFreehand() ?
       ExpandCelCanvas::UseModifiedRegionAsUndoInfo:
       ExpandCelCanvas::None)));

  if (!m_floodfillSrcImage)
    m_floodfillSrcImage = const_cast<Image*>(getSrcImage());

  // Settings
  switch (m_tool->getFill(m_button)) {
    case tools::FillNone:
      m_filled = false;
      break;
    case tools::FillAlways:
      m_filled = true;
      break;
    case tools::FillOptional:
      m_filled = config.filled;
      break;
  }

  if (m_ink->isSelection())
    m_useMask = false;
  else
    m_useMask = m_document->isMaskVisible();

  // Start with an empty mask if the user is selecting with "default selection mode"
  if (getInk()->isSelection() &&
      (!m_document->isMaskVisible() ||
       (int(getModifiers()) & int(tools::ToolLoopModifiers::kReplaceSelection)))) {
    Mask emptyMask;
    m_transaction.execute(new cmd::SetMask(m_document, &emptyMask));
  }

  m_celOrigin = m_expandCelCanvas->getCel()->position();
  m_mask = m_document->mask();
  m_maskOrigin = (!m_mask->isEmpty() ? gfx::Point(m_mask->bounds().x-m_celOrigin.x,
                                                  m_mask->bounds().y-m_celOrigin.y):
                                       gfx::Point(0, 0));
}

ScriptToolLoop::~ScriptToolLoop()
{
  if (m_floodfillSrcImage != getSrcImage())
    delete m_floodfillSrcImage;
}

void ScriptToolLoop::dispose()
{
  bool redraw = false;

  if (!m_canceled) {
    // Paint ink
    if (getInk()->isPaint()) {
      try {
        ContextReader reader(m_context, 500);
        ContextWriter writer(reader, 500);
        m_expandCelCanvas->commit();
      }
      catch (const LockedDocumentException& ex) {
        Console::showException(ex);
      }
    }
    // Selection ink
    else if (getInk()->isSelection()) {
      m_document->generateMaskBoundaries();
      redraw = true;

      // Show selection edges
      m_docPref.show.selectionEdges(true);
    }

    // Commit only our own transaction. When joining an external
    // transaction the caller commits it, producing the undo step.
    if (m_ownsTransaction)
      m_transaction.commit();
  }
  else
    redraw = true;

  // If the trace was canceled or it is not a 'paint' ink...
  if (m_canceled || !getInk()->isPaint()) {
    try {
      ContextReader reader(m_context, 500);
      ContextWriter writer(reader, 500);
      m_expandCelCanvas->rollback();
    }
    catch (const LockedDocumentException& ex) {
      Console::showException(ex);
    }
  }

  if (redraw)
    update_screen_for_document(m_document);
}

// tools::ToolLoop interface
tools::Tool* ScriptToolLoop::getTool() { return m_tool; }
doc::Brush* ScriptToolLoop::getBrush() { return m_brush.get(); }
Document* ScriptToolLoop::getDocument() { return m_document; }
doc::Sprite* ScriptToolLoop::sprite() { return m_sprite; }
doc::Layer* ScriptToolLoop::getLayer() { return m_layer; }
frame_t ScriptToolLoop::getFrame() { return m_frame; }
doc::RgbMap* ScriptToolLoop::getRgbMap() {
  if (!m_rgbMap) {
    Sprite::RgbMapFor forLayer =
      ((!m_layer ||
        m_layer->isBackground() ||
        m_sprite->pixelFormat() == IMAGE_RGB) ?
       Sprite::RgbMapFor::OpaqueLayer:
       Sprite::RgbMapFor::TransparentLayer);
    m_rgbMap = m_sprite->rgbMap(m_frame, forLayer);
  }
  return m_rgbMap;
}
const render::Zoom& ScriptToolLoop::zoom() { return m_zoom; }
tools::ToolLoop::Button ScriptToolLoop::getMouseButton() { return m_button; }
doc::color_t ScriptToolLoop::getFgColor() { return m_fgColor; }
doc::color_t ScriptToolLoop::getBgColor() { return m_bgColor; }
doc::color_t ScriptToolLoop::getPrimaryColor() { return m_primaryColor; }
void ScriptToolLoop::setPrimaryColor(doc::color_t color) { m_primaryColor = color; }
doc::color_t ScriptToolLoop::getSecondaryColor() { return m_secondaryColor; }
void ScriptToolLoop::setSecondaryColor(doc::color_t color) { m_secondaryColor = color; }
int ScriptToolLoop::getOpacity() { return m_opacity; }
int ScriptToolLoop::getTolerance() { return m_tolerance; }
bool ScriptToolLoop::getContiguous() { return m_contiguous; }
tools::ToolLoopModifiers ScriptToolLoop::getModifiers() {
  // The default selection mode replaces the selection (as the GUI),
  // so the selection ink adds to the mask in the final step.
  if (m_ink->isSelection())
    return tools::ToolLoopModifiers::kReplaceSelection;
  return tools::ToolLoopModifiers(0);
}
filters::TiledMode ScriptToolLoop::getTiledMode() { return filters::TiledMode::NONE; }
bool ScriptToolLoop::getGridVisible() { return false; }
bool ScriptToolLoop::getSnapToGrid() { return false; }
bool ScriptToolLoop::getStopAtGrid() { return false; }
gfx::Rect ScriptToolLoop::getGridBounds() { return gfx::Rect(0, 0, m_sprite->width(), m_sprite->height()); }
gfx::Point ScriptToolLoop::getCelOrigin() { return m_celOrigin; }
void ScriptToolLoop::setSpeed(const gfx::Point& speed) { m_speed = speed; }
gfx::Point ScriptToolLoop::getSpeed() { return m_speed; }
tools::Ink* ScriptToolLoop::getInk() { return m_ink.get(); }
tools::Controller* ScriptToolLoop::getController() { return m_controller; }
tools::PointShape* ScriptToolLoop::getPointShape() { return m_pointShape; }
tools::Intertwine* ScriptToolLoop::getIntertwine() { return m_intertwine; }
tools::TracePolicy ScriptToolLoop::getTracePolicy() { return m_tracePolicy; }
tools::Symmetry* ScriptToolLoop::getSymmetry() { return m_symmetry.get(); }
const doc::Remap* ScriptToolLoop::getShadingRemap() { return m_shadingRemap.get(); }

const doc::Image* ScriptToolLoop::getSrcImage() { return m_expandCelCanvas->getSourceCanvas(); }
const doc::Image* ScriptToolLoop::getFloodFillSrcImage() { return m_floodfillSrcImage; }
doc::Image* ScriptToolLoop::getDstImage() { return m_expandCelCanvas->getDestCanvas(); }
void ScriptToolLoop::validateSrcImage(const gfx::Region& rgn) {
  m_expandCelCanvas->validateSourceCanvas(rgn);
}
void ScriptToolLoop::validateDstImage(const gfx::Region& rgn) {
  m_expandCelCanvas->validateDestCanvas(rgn);
}
void ScriptToolLoop::invalidateDstImage() {
  m_expandCelCanvas->invalidateDestCanvas();
}
void ScriptToolLoop::invalidateDstImage(const gfx::Region& rgn) {
  m_expandCelCanvas->invalidateDestCanvas(rgn);
}
void ScriptToolLoop::copyValidDstToSrcImage(const gfx::Region& rgn) {
  m_expandCelCanvas->copyValidDestToSourceCanvas(rgn);
}

bool ScriptToolLoop::useMask() { return m_useMask; }
doc::Mask* ScriptToolLoop::getMask() { return m_mask; }
void ScriptToolLoop::setMask(doc::Mask* newMask) {
  m_transaction.execute(new cmd::SetMask(m_document, newMask));
}
gfx::Point ScriptToolLoop::getMaskOrigin() { return m_maskOrigin; }
bool ScriptToolLoop::getFilled() { return m_filled; }
bool ScriptToolLoop::getPreviewFilled() { return m_previewFilled; }
int ScriptToolLoop::getSprayWidth() { return m_sprayWidth; }
int ScriptToolLoop::getSpraySpeed() { return m_spraySpeed; }

gfx::Region& ScriptToolLoop::getDirtyArea() { return m_dirtyArea; }

void ScriptToolLoop::updateDirtyArea() {
  m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea, m_frame);
}

void ScriptToolLoop::updateStatusBar(const char*) { }

void ScriptToolLoop::cancel() { m_canceled = true; }
bool ScriptToolLoop::isCanceled() { return m_canceled; }

} // namespace app
