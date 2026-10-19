// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"
#include "app/color_target.h"
#include "app/pref/preferences.h"
#include "app/tools/tool_loop.h"
#include "doc/brush.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "render/zoom.h"

#include <memory>

namespace doc {
  class Layer;
  class Sprite;
  class Image;
  class Mask;
  class RgbMap;
  class Remap;
}

namespace app {
  class Context;
  class Document;
  class ExpandCelCanvas;
  class Transaction;

  namespace tools {
    class Tool;
    class Ink;
  }

  // Parameters to drive the GUI drawing pipeline without an Editor.
  // The constructor throws std::runtime_error when the config is invalid.
  //
  // The drawing parameters use fixed defaults; the user preferences are
  // not read, so the result is independent of the GUI setup.
  //
  // The target document must be the active document (UIContext::
  // setActiveDocument) before the loop is constructed, because the
  // Transaction reads the active document's undo history.
  struct ScriptToolLoopConfig {
    Context* context = nullptr;      // The UIContext
    Document* document = nullptr;
    doc::Layer* layer = nullptr;     // nullptr allowed for non-floodfill selection tools
    frame_t frame = 0;
    tools::Tool* tool = nullptr;
    std::shared_ptr<tools::Ink> inkOverride;  // nullptr = the tool default
    doc::BrushRef brush;                     // caller provides a valid brush
    Color fg;
    Color bg;
    int opacity = 255;
    bool filled = false;             // used by fill-optional tools
    bool previewFilled = false;
    int tolerance = 0;
    bool contiguous = true;
    int sprayWidth = 16;
    int spraySpeed = 32;
    tools::FreehandAlgorithm freehandAlgorithm = tools::FreehandAlgorithm::DEFAULT;
    gen::FillReferTo floodFillReferTo = gen::FillReferTo::ACTIVE_LAYER;
    tools::ToolLoop::Button button = tools::ToolLoop::Left;
    Transaction* externalTransaction = nullptr;  // nullptr = own a Transaction
    // An external transaction must outlive the loop
  };

  // A tools::ToolLoop that works without an Editor/StatusBar, so the
  // drawing pipeline can be driven by scripts (or other non-UI callers).
  // It mirrors the GUI ToolLoopImpl with the UI dependencies replaced.
  class ScriptToolLoop : public tools::ToolLoop {
  public:
    explicit ScriptToolLoop(const ScriptToolLoopConfig& config);
    ~ScriptToolLoop() override;

    // tools::ToolLoop interface
    void dispose() override;

    tools::Tool* getTool() override;
    doc::Brush* getBrush() override;
    Document* getDocument() override;
    doc::Sprite* sprite() override;
    doc::Layer* getLayer() override;
    frame_t getFrame() override;

    const doc::Image* getSrcImage() override;
    const doc::Image* getFloodFillSrcImage() override;
    doc::Image* getDstImage() override;
    void validateSrcImage(const gfx::Region& rgn) override;
    void validateDstImage(const gfx::Region& rgn) override;
    void invalidateDstImage() override;
    void invalidateDstImage(const gfx::Region& rgn) override;
    void copyValidDstToSrcImage(const gfx::Region& rgn) override;

    doc::RgbMap* getRgbMap() override;
    bool useMask() override;
    doc::Mask* getMask() override;
    void setMask(doc::Mask* newMask) override;
    gfx::Point getMaskOrigin() override;

    const render::Zoom& zoom() override;
    Button getMouseButton() override;

    doc::color_t getFgColor() override;
    doc::color_t getBgColor() override;
    doc::color_t getPrimaryColor() override;
    void setPrimaryColor(doc::color_t color) override;
    doc::color_t getSecondaryColor() override;
    void setSecondaryColor(doc::color_t color) override;

    int getOpacity() override;
    int getTolerance() override;
    bool getContiguous() override;
    tools::ToolLoopModifiers getModifiers() override;
    filters::TiledMode getTiledMode() override;

    bool getGridVisible() override;
    bool getSnapToGrid() override;
    bool getStopAtGrid() override;
    gfx::Rect getGridBounds() override;

    bool getFilled() override;
    bool getPreviewFilled() override;
    int getSprayWidth() override;
    int getSpraySpeed() override;

    gfx::Point getCelOrigin() override;
    void setSpeed(const gfx::Point& speed) override;
    gfx::Point getSpeed() override;

    tools::Ink* getInk() override;
    tools::Controller* getController() override;
    tools::PointShape* getPointShape() override;
    tools::Intertwine* getIntertwine() override;
    tools::TracePolicy getTracePolicy() override;
    tools::Symmetry* getSymmetry() override;

    const doc::Remap* getShadingRemap() override;

    void cancel() override;
    bool isCanceled() override;
    gfx::Region& getDirtyArea() override;
    void updateDirtyArea() override;
    void updateStatusBar(const char* text) override;

  private:
    static Context* validateConfig(const ScriptToolLoopConfig& config);

    Context* m_context;
    tools::Tool* m_tool;
    doc::BrushRef m_brush;
    Document* m_document;
    doc::Sprite* m_sprite;
    doc::Layer* m_layer;
    frame_t m_frame;
    doc::RgbMap* m_rgbMap;
    DocumentPreferences& m_docPref;
    int m_opacity;
    int m_tolerance;
    bool m_contiguous;
    render::Zoom m_zoom;
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
    ColorTarget m_colorTarget;
    doc::color_t m_fgColor;
    doc::color_t m_bgColor;
    doc::color_t m_primaryColor;
    doc::color_t m_secondaryColor;
    gfx::Region m_dirtyArea;

    bool m_filled;
    bool m_previewFilled;
    int m_sprayWidth;
    int m_spraySpeed;
    bool m_useMask;
    doc::Mask* m_mask;
    gfx::Point m_maskOrigin;
    bool m_canceled;
    bool m_ownsTransaction;
    std::unique_ptr<Transaction> m_ownedTransaction;
    Transaction& m_transaction;
    std::unique_ptr<ExpandCelCanvas> m_expandCelCanvas;
    doc::Image* m_floodfillSrcImage;
  };

} // namespace app
