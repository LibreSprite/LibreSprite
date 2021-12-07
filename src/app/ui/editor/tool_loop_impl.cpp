// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/tool_loop_impl.h"

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document_undo.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/controller.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/symmetry.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"
#include "doc/sprite.h"
#include "render/render.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////
// Common properties between drawing/preview ToolLoop impl

class ToolLoopBase : public tools::ToolLoop {

protected:
  Editor* m_editor;
  tools::Tool* m_tool;
  BrushRef m_brush;
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  RgbMap* m_rgbMap;
  DocumentPreferences& m_docPref;
  ToolPreferences& m_toolPref;
  int m_opacity;
  int m_tolerance;
  bool m_contiguous;
  gfx::Point m_celOrigin;
  gfx::Point m_speed;
  tools::ToolLoop::Button m_button;
  std::unique_ptr<tools::Ink> m_ink;
  tools::Controller* m_controller;
  tools::PointShape* m_pointShape;
  tools::Intertwine* m_intertwine;
  tools::TracePolicy m_tracePolicy;
  std::unique_ptr<tools::Symmetry> m_symmetry;
  std::unique_ptr<doc::Remap> m_shadingRemap;
  app::ColorTarget m_colorTarget;
  doc::color_t m_fgColor;
  doc::color_t m_bgColor;
  doc::color_t m_primaryColor;
  doc::color_t m_secondaryColor;
  gfx::Region m_dirtyArea;

public:
  ToolLoopBase(Editor* editor,
               Layer* layer,
               tools::Tool* tool,
               std::shared_ptr<tools::Ink> ink,
               Document* document,
               tools::ToolLoop::Button button,
               const app::Color& fgColor,
               const app::Color& bgColor)
    : m_editor(editor)
    , m_tool(tool)
    , m_brush(App::instance()->contextBar()->activeBrush(m_tool))
    , m_document(document)
    , m_sprite(editor->sprite())
    , m_layer(layer)
    , m_frame(editor->frame())
    , m_rgbMap(nullptr)
    , m_docPref(Preferences::instance().document(m_document))
    , m_toolPref(Preferences::instance().tool(m_tool))
    , m_opacity(m_toolPref.opacity())
    , m_tolerance(m_toolPref.tolerance())
    , m_contiguous(m_toolPref.contiguous())
    , m_button(button)
    , m_ink(ink->clone())
    , m_controller(m_tool->getController(m_button))
    , m_pointShape(m_tool->getPointShape(m_button))
    , m_intertwine(m_tool->getIntertwine(m_button))
    , m_tracePolicy(m_tool->getTracePolicy(m_button))
    , m_symmetry(nullptr)
    , m_colorTarget(m_layer ? ColorTarget(m_layer):
                              ColorTarget(ColorTarget::BackgroundLayer,
                                          m_sprite->pixelFormat(),
                                          m_sprite->transparentColor()))
    , m_fgColor(color_utils::color_for_target_mask(fgColor, m_colorTarget))
    , m_bgColor(color_utils::color_for_target_mask(bgColor, m_colorTarget))
    , m_primaryColor(button == tools::ToolLoop::Left ? m_fgColor: m_bgColor)
    , m_secondaryColor(button == tools::ToolLoop::Left ? m_bgColor: m_fgColor)
  {
    tools::FreehandAlgorithm algorithm = m_toolPref.freehandAlgorithm();

    if (m_tracePolicy == tools::TracePolicy::Accumulate ||
        m_tracePolicy == tools::TracePolicy::AccumulateUpdateLast) {
      tools::ToolBox* toolbox = App::instance()->toolBox();

      switch (algorithm) {
        case tools::FreehandAlgorithm::DEFAULT:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines);
          m_tracePolicy = tools::TracePolicy::Accumulate;
          break;
        case tools::FreehandAlgorithm::PIXEL_PERFECT:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect);
          m_tracePolicy = tools::TracePolicy::AccumulateUpdateLast;
          break;
        case tools::FreehandAlgorithm::DOTS:
          m_intertwine = toolbox->getIntertwinerById(tools::WellKnownIntertwiners::None);
          m_tracePolicy = tools::TracePolicy::Accumulate;
          break;
      }
    }

    // Symmetry mode
    if (Preferences::instance().symmetryMode.enabled()) {
      switch (m_docPref.symmetry.mode()) {

        case app::gen::SymmetryMode::NONE:
          ASSERT(m_symmetry == nullptr);
          break;

        case app::gen::SymmetryMode::HORIZONTAL:
          m_symmetry.reset(new app::tools::HorizontalSymmetry(m_docPref.symmetry.xAxis()));
          break;

        case app::gen::SymmetryMode::VERTICAL:
          m_symmetry.reset(new app::tools::VerticalSymmetry(m_docPref.symmetry.yAxis()));
          break;

        case app::gen::SymmetryMode::BOTH:
          m_symmetry.reset(new app::tools::DoubleSymmetry(m_docPref.symmetry.xAxis(), m_docPref.symmetry.yAxis()));
          break;
      }
    }

    // Ignore opacity for these inks
    if (!tools::inkHasOpacity(m_toolPref.ink()) &&
        m_brush->type() != kImageBrushType &&
        !m_ink->isEffect()) {
      m_opacity = 255;
    }

    if (m_toolPref.ink() == tools::InkType::SHADING) {
      m_shadingRemap.reset(
        App::instance()->contextBar()->createShadeRemap(
          button == tools::ToolLoop::Left));
    }
  }

  // IToolLoop interface
  tools::Tool* getTool() override { return m_tool; }
  Brush* getBrush() override { return m_brush.get(); }
  Document* getDocument() override { return m_document; }
  Sprite* sprite() override { return m_sprite; }
  Layer* getLayer() override { return m_layer; }
  frame_t getFrame() override { return m_frame; }
  RgbMap* getRgbMap() override {
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
  const render::Zoom& zoom() override { return m_editor->zoom(); }
  ToolLoop::Button getMouseButton() override { return m_button; }
  doc::color_t getFgColor() override { return m_fgColor; }
  doc::color_t getBgColor() override { return m_bgColor; }
  doc::color_t getPrimaryColor() override { return m_primaryColor; }
  void setPrimaryColor(doc::color_t color) override { m_primaryColor = color; }
  doc::color_t getSecondaryColor() override { return m_secondaryColor; }
  void setSecondaryColor(doc::color_t color) override { m_secondaryColor = color; }
  int getOpacity() override { return m_opacity; }
  int getTolerance() override { return m_tolerance; }
  bool getContiguous() override { return m_contiguous; }
  tools::ToolLoopModifiers getModifiers() override { return m_editor->getToolLoopModifiers(); }
  filters::TiledMode getTiledMode() override { return m_docPref.tiled.mode(); }
  bool getGridVisible() override { return m_docPref.show.grid(); }
  bool getSnapToGrid() override { return m_docPref.grid.snap(); }
  bool getStopAtGrid() override {
    switch (m_toolPref.floodfill.stopAtGrid()) {
      case app::gen::StopAtGrid::NEVER:
        return false;
      case app::gen::StopAtGrid::IF_VISIBLE:
        return m_docPref.show.grid();
      case app::gen::StopAtGrid::ALWAYS:
        return true;
    }
    return false;
  }
  gfx::Rect getGridBounds() override { return m_docPref.grid.bounds(); }
  gfx::Point getCelOrigin() override { return m_celOrigin; }
  void setSpeed(const gfx::Point& speed) override { m_speed = speed; }
  gfx::Point getSpeed() override { return m_speed; }
  tools::Ink* getInk() override { return m_ink.get(); }
  tools::Controller* getController() override { return m_controller; }
  tools::PointShape* getPointShape() override { return m_pointShape; }
  tools::Intertwine* getIntertwine() override { return m_intertwine; }
  tools::TracePolicy getTracePolicy() override { return m_tracePolicy; }
  tools::Symmetry* getSymmetry() override { return m_symmetry.get(); }
  doc::Remap* getShadingRemap() override { return m_shadingRemap.get(); }

  gfx::Region& getDirtyArea() override {
    return m_dirtyArea;
  }

  void updateDirtyArea() override {
    // TODO find a way to avoid calling hide/show brush preview here
    HideBrushPreview hide(m_editor->brushPreview());
    m_document->notifySpritePixelsModified(m_sprite, m_dirtyArea,
                                           m_frame);
  }

  void updateStatusBar(const char* text) override {
    StatusBar::instance()->setStatusText(0, text);
  }

};

//////////////////////////////////////////////////////////////////////
// For drawing

class ToolLoopImpl : public ToolLoopBase {
  Context* m_context;
  bool m_filled;
  bool m_previewFilled;
  int m_sprayWidth;
  int m_spraySpeed;
  bool m_useMask;
  Mask* m_mask;
  gfx::Point m_maskOrigin;
  bool m_canceled;
  Transaction m_transaction;
  ExpandCelCanvas* m_expandCelCanvas;
  Image* m_floodfillSrcImage;

public:
  ToolLoopImpl(Editor* editor,
               Layer* layer,
               Context* context,
               tools::Tool* tool,
               std::shared_ptr<tools::Ink> ink,
               Document* document,
               tools::ToolLoop::Button button,
               const app::Color& fgColor,
               const app::Color& bgColor)
    : ToolLoopBase(editor, layer, tool, ink, document,
                   button, fgColor, bgColor)
    , m_context(context)
    , m_canceled(false)
    , m_transaction(m_context,
                    m_tool->getText().c_str(),
                    ((getInk()->isSelection() ||
                      getInk()->isEyedropper() ||
                      getInk()->isScrollMovement() ||
                      getInk()->isSlice() ||
                      getInk()->isZoom()) ? DoesntModifyDocument:
                                            ModifyDocument))
    , m_expandCelCanvas(nullptr)
    , m_floodfillSrcImage(nullptr)
  {
    ASSERT(m_context->activeDocument() == m_editor->document());

    if (m_pointShape->isFloodFill()) {
      // Prepare a special image for floodfill when it's configured to
      // stop using all visible layers.
      if (m_toolPref.floodfill.referTo() == gen::FillReferTo::ALL_LAYERS) {
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
        Cel* cel = m_layer->cel(m_frame);
        if (cel && (cel->x() != 0 || cel->y() != 0)) {
          m_floodfillSrcImage = Image::create(m_sprite->pixelFormat(),
                                              m_sprite->width(),
                                              m_sprite->height());
          m_floodfillSrcImage->clear(m_sprite->transparentColor());
          copy_image(m_floodfillSrcImage, cel->image(), cel->x(), cel->y());
        }
      }
    }

    m_expandCelCanvas = new ExpandCelCanvas(
      editor->getSite(),
      layer,
      m_docPref.tiled.mode(),
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
    switch (tool->getFill(m_button)) {
      case tools::FillNone:
        m_filled = false;
        break;
      case tools::FillAlways:
        m_filled = true;
        break;
      case tools::FillOptional:
        m_filled = m_toolPref.filled();
        break;
    }

    m_previewFilled = m_toolPref.filledPreview();
    m_sprayWidth = m_toolPref.spray.width();
    m_spraySpeed = m_toolPref.spray.speed();

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

  ~ToolLoopImpl() {
    if (m_floodfillSrcImage != getSrcImage())
      delete m_floodfillSrcImage;
    delete m_expandCelCanvas;
  }

  // IToolLoop interface
  void dispose() override
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

  const Image* getSrcImage() override { return m_expandCelCanvas->getSourceCanvas(); }
  const Image* getFloodFillSrcImage() override { return m_floodfillSrcImage; }
  Image* getDstImage() override { return m_expandCelCanvas->getDestCanvas(); }
  void validateSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->validateSourceCanvas(rgn);
  }
  void validateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->validateDestCanvas(rgn);
  }
  void invalidateDstImage() override {
    m_expandCelCanvas->invalidateDestCanvas();
  }
  void invalidateDstImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->invalidateDestCanvas(rgn);
  }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override {
    m_expandCelCanvas->copyValidDestToSourceCanvas(rgn);
  }

  bool useMask() override { return m_useMask; }
  Mask* getMask() override { return m_mask; }
  void setMask(Mask* newMask) override {
    m_transaction.execute(new cmd::SetMask(m_document, newMask));
  }
  gfx::Point getMaskOrigin() override { return m_maskOrigin; }
  bool getFilled() override { return m_filled; }
  bool getPreviewFilled() override { return m_previewFilled; }
  int getSprayWidth() override { return m_sprayWidth; }
  int getSpraySpeed() override { return m_spraySpeed; }

  void cancel() override { m_canceled = true; }
  bool isCanceled() override { return m_canceled; }

};

tools::ToolLoop* create_tool_loop(Editor* editor, Context* context)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  std::shared_ptr<tools::Ink> current_ink = editor->getCurrentEditorInk();
  if (!current_tool || !current_ink)
    return nullptr;

  Layer* layer;

  // For selection tools, we can use any layer (even without layers at
  // all), so we specify a nullptr here as the active layer. This is
  // used as a special case by the render::Render class to show the
  // preview image/selection stroke as a xor'd overlay in the render
  // result.
  //
  // Anyway this cannot be used in 'magic wand' tool (isSelection +
  // isFloodFill) because we need the original layer source
  // image/pixels to stop the flood-fill algorithm.
  if (current_ink->isSelection() &&
      !current_tool->getPointShape(editor->isSecondaryButton() ? 1: 0)->isFloodFill()) {
    layer = nullptr;
  }
  else {
    layer = editor->layer();
    if (!layer) {
      StatusBar::instance()->showTip(
        1000, "There is no active layer");
      return nullptr;
    }
    else if (!layer->isVisible()) {
      StatusBar::instance()->showTip(
        1000, "Layer '%s' is hidden", layer->name().c_str());
      return nullptr;
    }
    // If the active layer is read-only.
    else if (!layer->isEditable()) {
      StatusBar::instance()->showTip(
        1000, "Layer '%s' is locked", layer->name().c_str());
      return nullptr;
    }
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  app::Color fg = colorbar->getFgColor();
  app::Color bg = colorbar->getBgColor();

  ASSERT(fg.isValid());
  ASSERT(bg.isValid());

  if (!fg.isValid() || !bg.isValid()) {
    Alert::show(PACKAGE
                "<<The current selected foreground and/or background color"
                "<<is out of range. Select a valid color in the color-bar."
                "||&Close");
    return NULL;
  }

  // Create the new tool loop
  try {
    return new ToolLoopImpl(
      editor, layer, context,
      current_tool,
      current_ink,
      editor->document(),
      !editor->isSecondaryButton() ? tools::ToolLoop::Left: tools::ToolLoop::Right,
      fg, bg);
  }
  catch (const std::exception& ex) {
    Alert::show(PACKAGE
                "<<Error drawing ink:"
                "<<%s"
                "||&Close",
                ex.what());
    return NULL;
  }
}

//////////////////////////////////////////////////////////////////////
// For preview

class PreviewToolLoopImpl : public ToolLoopBase {
  Image* m_image;

public:
  PreviewToolLoopImpl(
    Editor* editor,
    tools::Tool* tool,
    std::shared_ptr<tools::Ink> ink,
    Document* document,
    const app::Color& fgColor,
    const app::Color& bgColor,
    Image* image,
    const gfx::Point& celOrigin)
    : ToolLoopBase(editor, editor->layer(), tool, ink, document,
                   tools::ToolLoop::Left, fgColor, bgColor)
    , m_image(image)
  {
    m_celOrigin = celOrigin;

    // Avoid preview for spray and flood fill like tools
    if (m_pointShape->isSpray()) {
      m_pointShape = App::instance()->toolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Brush);
    }
    else if (m_pointShape->isFloodFill()) {
      m_pointShape = App::instance()->toolBox()->getPointShapeById(
        tools::WellKnownPointShapes::Pixel);
    }
  }

  // IToolLoop interface
  void dispose() override { }
  const Image* getSrcImage() override { return m_image; }
  const Image* getFloodFillSrcImage() override { return m_image; }
  Image* getDstImage() override { return m_image; }
  void validateSrcImage(const gfx::Region& rgn) override { }
  void validateDstImage(const gfx::Region& rgn) override { }
  void invalidateDstImage() override { }
  void invalidateDstImage(const gfx::Region& rgn) override { }
  void copyValidDstToSrcImage(const gfx::Region& rgn) override { }

  bool useMask() override { return false; }
  Mask* getMask() override { return nullptr; }
  void setMask(Mask* newMask) override { }
  gfx::Point getMaskOrigin() override { return gfx::Point(0, 0); }
  bool getFilled() override { return false; }
  bool getPreviewFilled() override { return false; }
  int getSprayWidth() override { return 0; }
  int getSpraySpeed() override { return 0; }

  void cancel() override { }
  bool isCanceled() override { return true; }

};

tools::ToolLoop* create_tool_loop_preview(
  Editor* editor, Image* image,
  const gfx::Point& celOrigin)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  std::shared_ptr<tools::Ink> current_ink = editor->getCurrentEditorInk();
  if (!current_tool || !current_ink)
    return nullptr;

  Layer* layer = editor->layer();
  if (!layer ||
      !layer->isVisible() ||
      !layer->isEditable()) {
    return nullptr;
  }

  // Get fg/bg colors
  ColorBar* colorbar = ColorBar::instance();
  app::Color fg = colorbar->getFgColor();
  app::Color bg = colorbar->getBgColor();
  if (!fg.isValid() || !bg.isValid())
    return nullptr;

  // Create the new tool loop
  try {
    return new PreviewToolLoopImpl(
      editor,
      current_tool,
      current_ink,
      editor->document(),
      fg, bg, image, celOrigin);
  }
  catch (const std::exception&) {
    return nullptr;
  }
}

//////////////////////////////////////////////////////////////////////

} // namespace app
