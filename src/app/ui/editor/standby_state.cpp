// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/standby_state.h"

#include "app/app.h"
#include "app/color_picker.h"
#include "app/commands/cmd_eyedropper.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document_range.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/pick_ink.h"
#include "app/tools/tool.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/drawing_state.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/moving_cel_state.h"
#include "app/ui/editor/moving_pixels_state.h"
#include "app/ui/editor/moving_symmetry_state.h"
#include "app/ui/editor/pivot_helpers.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/scrolling_state.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/editor/transform_handles.h"
#include "app/ui/editor/zooming_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"
#include "base/bind.h"
#include "base/pi.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "fixmath/fixmath.h"
#include "gfx/rect.h"
#include "she/surface.h"
#include "ui/alert.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <cmath>
#include <cstring>

namespace app {

using namespace ui;

static CursorType rotated_size_cursors[] = {
  kSizeECursor,
  kSizeNECursor,
  kSizeNCursor,
  kSizeNWCursor,
  kSizeWCursor,
  kSizeSWCursor,
  kSizeSCursor,
  kSizeSECursor
};

static CursorType rotated_rotate_cursors[] = {
  kRotateECursor,
  kRotateNECursor,
  kRotateNCursor,
  kRotateNWCursor,
  kRotateWCursor,
  kRotateSWCursor,
  kRotateSCursor,
  kRotateSECursor
};

#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif

StandbyState::StandbyState()
  : m_decorator(new Decorator(this))
  , m_transformSelectionHandlesAreVisible(false)
{
}

StandbyState::~StandbyState()
{
  delete m_decorator;
}

void StandbyState::onEnterState(Editor* editor)
{
  StateWithWheelBehavior::onEnterState(editor);

  editor->setDecorator(m_decorator);

  m_pivotVisConn =
    Preferences::instance().selection.pivotVisibility.AfterChange.connect(
      base::Bind<void>(&StandbyState::onPivotChange, this, editor));
  m_pivotPosConn =
    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      base::Bind<void>(&StandbyState::onPivotChange, this, editor));
}

void StandbyState::onActiveToolChange(Editor* editor, tools::Tool* tool)
{
  // If the user change from a selection tool to a non-selection tool,
  // or viceversa, we've to show or hide the transformation handles.
  bool needDecorators = (tool && tool->getInk(0)->isSelection());
  if (m_transformSelectionHandlesAreVisible != needDecorators) {
    m_transformSelectionHandlesAreVisible = false;
    editor->invalidate();
  }
}

bool StandbyState::checkForScroll(Editor* editor, MouseMessage* msg)
{
  tools::Ink* clickedInk = editor->getCurrentEditorInk().get();

  // Start scroll loop
  if (msg->middle() || clickedInk->isScrollMovement()) { // TODO msg->middle() should be customizable
    EditorStatePtr newState(new ScrollingState());
    editor->setState(newState);

    newState->onMouseDown(editor, msg);
    return true;
  }
  else
    return false;
}

bool StandbyState::checkForZoom(Editor* editor, MouseMessage* msg)
{
  tools::Ink* clickedInk = editor->getCurrentEditorInk().get();

  // Start scroll loop
  if (clickedInk->isZoom()) {
    EditorStatePtr newState(new ZoomingState());
    editor->setState(newState);

    newState->onMouseDown(editor, msg);
    return true;
  }
  else
    return false;
}

bool StandbyState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return true;

  UIContext* context = UIContext::instance();
  std::shared_ptr<tools::Ink> clickedInk = editor->getCurrentEditorInk();
  Site site;
  editor->getSite(&site);
  app::Document* document = static_cast<app::Document*>(site.document());
  Layer* layer = site.layer();

  // When an editor is clicked the current view is changed.
  context->setActiveView(editor->getDocumentView());

  // Start scroll loop
  if (checkForScroll(editor, msg) || checkForZoom(editor, msg))
    return true;

  // Move cel X,Y coordinates
  if (clickedInk->isCelMovement()) {
    // Handle "Auto Select Layer"
    if (editor->isAutoSelectLayer()) {
      gfx::Point cursor = editor->screenToEditor(msg->position());

      ColorPicker picker;
      picker.pickColor(site, cursor, ColorPicker::FromComposition);

      auto range = App::instance()->timeline()->range();

      // Change layer only when the layer is diffrent from current one, and
      // the range we selected is not with multiple cels.
      bool layerChanged = (layer != picker.layer());
      bool rangeEnabled = range.enabled();
      bool rangeSingleCel = ((range.type() == DocumentRange::kCels) &&
                             (range.layers() == 1) && (range.frames() == 1));

      if (layerChanged && (!rangeEnabled || rangeSingleCel)) {
        layer = picker.layer();
        if (layer) {
          editor->setLayer(layer);
          editor->flashCurrentLayer();
        }
      }
    }

    if ((layer) &&
        (layer->type() == ObjectType::LayerImage)) {
      // TODO we should be able to move the `Background' with tiled mode
      if (layer->isBackground()) {
        StatusBar::instance()->showTip(1000,
          "The background layer cannot be moved");
      }
      else if (!layer->isVisible()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is hidden", layer->name().c_str());
      }
      else if (!layer->isMovable() || !layer->isEditable()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is locked", layer->name().c_str());
      }
      else if (!layer->cel(editor->frame())) {
        StatusBar::instance()->showTip(1000,
          "Cel is empty, nothing to move");
      }
      else {
        // Change to MovingCelState
        editor->setState(EditorStatePtr(new MovingCelState(editor, msg)));
      }
    }

    return true;
  }

  // Call the eyedropper command
  if (clickedInk->isEyedropper()) {
    editor->captureMouse();
    callEyedropper(editor);
    return true;
  }

  if (clickedInk->isSelection()) {
    // Transform selected pixels
    if (editor->isActive() &&
        document->isMaskVisible() &&
        m_decorator->getTransformHandles(editor)) {
      TransformHandles* transfHandles = m_decorator->getTransformHandles(editor);

      // Get the handle covered by the mouse.
      HandleType handle = transfHandles->getHandleAtPoint(editor,
        msg->position(),
        document->getTransformation());

      if (handle != NoHandle) {
        int x, y, opacity;
        Image* image = site.image(&x, &y, &opacity);
        if (layer && image) {
          if (!layer->isEditable()) {
            StatusBar::instance()->showTip(1000,
              "Layer '%s' is locked", layer->name().c_str());
            return true;
          }

          // Change to MovingPixelsState
          transformSelection(editor, msg, handle);
        }
        return true;
      }
    }

    // Move selected pixels
    if (layer && editor->isInsideSelection() && msg->left()) {
      if (!layer->isEditable()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is locked", layer->name().c_str());
        return true;
      }

      // Change to MovingPixelsState
      transformSelection(editor, msg, MoveHandle);
      return true;
    }
  }

  // Move symmetry
  SymmetryHandles handles;
  if (m_decorator->getSymmetryHandles(editor, handles)) {
    for (auto& handle : handles) {
      if (handle.rect().contains(msg->position())) {
        auto& symmetry = Preferences::instance().document(editor->document()).symmetry;
        const auto& axis = handle.axis();
        auto& axisPos = axis == Axis::HORIZONTAL ? symmetry.xAxis : symmetry.yAxis;

        editor->setState(
          EditorStatePtr(new MovingSymmetryState(editor, msg, axis, axisPos)));
        return true;
      }
    }
  }

  // Start the Tool-Loop
  if (layer) {
    // Disable layer edges to avoid showing the modified cel
    // information by ExpandCelCanvas (i.e. the cel origin is changed
    // to 0,0 coordinate.)
    auto& layerEdgesOption = editor->docPref().show.layerEdges;
    bool layerEdges = layerEdgesOption();
    if (layerEdges)
      layerEdgesOption(false);

    tools::ToolLoop* toolLoop = create_tool_loop(editor, context);
    if (toolLoop) {
      EditorStatePtr newState(new DrawingState(toolLoop));
      editor->setState(newState);

      static_cast<DrawingState*>(newState.get())
        ->initToolLoop(editor, msg);
    }

    // Restore layer edges
    if (layerEdges)
      layerEdgesOption(true);
    return true;
  }

  return true;
}

bool StandbyState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->releaseMouse();
  return true;
}

bool StandbyState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  // We control eyedropper tool from here. TODO move this to another place
  if (msg->left() || msg->right()) {
    tools::Ink* clickedInk = editor->getCurrentEditorInk().get();
    if (clickedInk->isEyedropper() &&
        editor->hasCapture()) {
      callEyedropper(editor);
    }
  }

  editor->showBrushPreview(msg->position());
  editor->updateStatusBar();
  return true;
}

bool StandbyState::onDoubleClick(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return false;

  tools::Ink* ink = editor->getCurrentEditorInk().get();

  // Select a tile with double-click
  if (ink->isSelection()) {
    Command* selectTileCmd =
      CommandsModule::instance()->getCommandByName(CommandId::SelectTile);

    Params params;
    if (int(editor->getToolLoopModifiers()) & int(tools::ToolLoopModifiers::kAddSelection))
      params.set("mode", "add");
    else if (int(editor->getToolLoopModifiers()) & int(tools::ToolLoopModifiers::kSubtractSelection))
      params.set("mode", "subtract");

    UIContext::instance()->executeCommand(selectTileCmd, params);
    return true;
  }

  return false;
}

bool StandbyState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = editor->getCurrentEditorInk().get();

  // See if the cursor is in some selection handle.
  if (m_decorator->onSetCursor(ink, editor, mouseScreenPos))
    return true;

  if (ink) {
    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (ink->isSelection()) {
      // Move pixels
      if (editor->isInsideSelection()) {
        EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
        if ((customization) &&
            int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection))
          editor->showMouseCursor(kArrowPlusCursor);
        else
          editor->showMouseCursor(kMoveCursor);
      }
      else
        editor->showBrushPreview(mouseScreenPos);
      return true;
    }
    else if (ink->isEyedropper()) {
      editor->showMouseCursor(kEyedropperCursor);
      return true;
    }
    else if (ink->isZoom()) {
      editor->showMouseCursor(kMagnifierCursor);
      return true;
    }
    else if (ink->isScrollMovement()) {
      editor->showMouseCursor(kScrollCursor);
      return true;
    }
    else if (ink->isCelMovement()) {
      editor->showMouseCursor(kMoveCursor);
      return true;
    }
    else if (ink->isSlice()) {
      editor->showBrushPreview(mouseScreenPos);
      return true;
    }
  }

  // Draw
  if (editor->canDraw()) {
    editor->showBrushPreview(mouseScreenPos);
  }
  // Forbidden
  else {
    editor->showMouseCursor(kForbiddenCursor);
  }

  return true;
}

bool StandbyState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool StandbyState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool StandbyState::onUpdateStatusBar(Editor* editor)
{
  tools::Ink* ink = editor->getCurrentEditorInk().get();
  const Sprite* sprite = editor->sprite();
  gfx::Point spritePos = editor->screenToEditor(ui::get_mouse_position());

  if (!sprite) {
    StatusBar::instance()->clearText();
  }
  // For eye-dropper
  else if (ink->isEyedropper()) {
    EyedropperCommand cmd;
    app::Color color = Preferences::instance().colorBar.fgColor();
    cmd.pickSample(editor->getSite(), spritePos, color);

    char buf[256];
    sprintf(buf, " :pos: %d %d", spritePos.x, spritePos.y);

    StatusBar::instance()->showColor(0, buf, color);
  }
  else {
    Mask* mask =
      (editor->document()->isMaskVisible() ?
       editor->document()->mask(): NULL);

    char buf[1024];
    sprintf(
      buf, ":pos: %d %d :%s: %d %d",
      spritePos.x, spritePos.y,
      (mask ? "selsize": "size"),
      (mask ? mask->bounds().w: sprite->width()),
      (mask ? mask->bounds().h: sprite->height()));

    if (sprite->totalFrames() > 1) {
      sprintf(
        buf+std::strlen(buf), " :frame: %d :clock: %d",
        editor->frame()+1,
        sprite->frameDuration(editor->frame()));
    }

    if (editor->docPref().show.grid()) {
      auto gb = editor->docPref().grid.bounds();
      int col = (spritePos.x - (gb.x % gb.w)) / gb.w;
      int row = (spritePos.y - (gb.y % gb.h)) / gb.h;
      sprintf(
        buf+std::strlen(buf), " :grid: %d %d", col, row);
    }

    StatusBar::instance()->setStatusText(0, buf);
  }

  return true;
}

Transformation StandbyState::getTransformation(Editor* editor)
{
  Transformation t = editor->document()->getTransformation();
  set_pivot_from_preferences(t);
  return t;
}

void StandbyState::startSelectionTransformation(Editor* editor,
                                                const gfx::Point& move,
                                                double angle)
{
  transformSelection(editor, NULL, NoHandle);

  if (MovingPixelsState* movingPixels = dynamic_cast<MovingPixelsState*>(editor->getState().get())) {
    movingPixels->translate(move);
    if (std::fabs(angle) > 1e-5)
      movingPixels->rotate(angle);
  }
}

void StandbyState::transformSelection(Editor* editor, MouseMessage* msg, HandleType handle)
{
  Document* document = editor->document();

  for (auto docView : UIContext::instance()->getAllDocumentViews(document)) {
    if (docView->editor()->isMovingPixels()) {
      // TODO Transfer moving pixels state to this editor
      docView->editor()->dropMovingPixels();
    }
  }

  try {
    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();

    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
    std::unique_ptr<Image> tmpImage(new_image_from_mask(editor->getSite()));

    PixelsMovementPtr pixelsMovement(
      new PixelsMovement(UIContext::instance(),
                         editor->getSite(),
                         tmpImage.get(),
                         document->mask(),
                         "Transformation"));

    // If the Ctrl key is pressed start dragging a copy of the selection
    if ((customization) &&
        int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection))
      pixelsMovement->copyMask();
    else
      pixelsMovement->cutMask();

    editor->setState(EditorStatePtr(new MovingPixelsState(editor, msg, pixelsMovement, handle)));
  }
  catch (const LockedDocumentException&) {
    // Other editor is locking the document.

    // TODO steal the PixelsMovement of the other editor and use it for this one.
    StatusBar::instance()->showTip(1000, "The sprite is locked in other editor");
    editor->showMouseCursor(kForbiddenCursor);
  }
  catch (const std::bad_alloc&) {
    StatusBar::instance()->showTip(1000, "Not enough memory to transform the selection");
    editor->showMouseCursor(kForbiddenCursor);
  }
}

void StandbyState::callEyedropper(Editor* editor)
{
  tools::Ink* clickedInk = editor->getCurrentEditorInk().get();
  Command* eyedropper_cmd =
    CommandsModule::instance()->getCommandByName(CommandId::Eyedropper);
  bool fg = (static_cast<tools::PickInk*>(clickedInk)->target() == tools::PickInk::Fg);

  Params params;
  params.set("target", fg ? "foreground": "background");

  UIContext::instance()->executeCommand(eyedropper_cmd, params);
}

void StandbyState::onPivotChange(Editor* editor)
{
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowMask &&
      editor->document()->isMaskVisible() &&
      !editor->document()->mask()->isFrozen()) {
    editor->invalidate();
  }
}

//////////////////////////////////////////////////////////////////////
// Decorator

StandbyState::Decorator::Decorator(StandbyState* standbyState)
  : m_transfHandles(NULL)
  , m_standbyState(standbyState)
{
}

StandbyState::Decorator::~Decorator()
{
  delete m_transfHandles;
}

TransformHandles* StandbyState::Decorator::getTransformHandles(Editor* editor)
{
  if (!m_transfHandles)
    m_transfHandles = new TransformHandles();

  return m_transfHandles;
}

bool StandbyState::Decorator::onSetCursor(tools::Ink* ink, Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (!editor->isActive())
    return false;

  if (ink && ink->isSelection() && editor->document()->isMaskVisible()) {
    const Transformation transformation(m_standbyState->getTransformation(editor));
    TransformHandles* tr = getTransformHandles(editor);
    HandleType handle = tr->getHandleAtPoint(
      editor, mouseScreenPos, transformation);

    CursorType newCursor = kArrowCursor;

    switch (handle) {
      case ScaleNWHandle:         newCursor = kSizeNWCursor; break;
      case ScaleNHandle:          newCursor = kSizeNCursor; break;
      case ScaleNEHandle:         newCursor = kSizeNECursor; break;
      case ScaleWHandle:          newCursor = kSizeWCursor; break;
      case ScaleEHandle:          newCursor = kSizeECursor; break;
      case ScaleSWHandle:         newCursor = kSizeSWCursor; break;
      case ScaleSHandle:          newCursor = kSizeSCursor; break;
      case ScaleSEHandle:         newCursor = kSizeSECursor; break;
      case RotateNWHandle:        newCursor = kRotateNWCursor; break;
      case RotateNHandle:         newCursor = kRotateNCursor; break;
      case RotateNEHandle:        newCursor = kRotateNECursor; break;
      case RotateWHandle:         newCursor = kRotateWCursor; break;
      case RotateEHandle:         newCursor = kRotateECursor; break;
      case RotateSWHandle:        newCursor = kRotateSWCursor; break;
      case RotateSHandle:         newCursor = kRotateSCursor; break;
      case RotateSEHandle:        newCursor = kRotateSECursor; break;
      case PivotHandle:           newCursor = kHandCursor; break;
      default:
        return false;
    }

    // Adjust the cursor depending the current transformation angle.
    fixmath::fixed angle = fixmath::ftofix(128.0 * transformation.angle() / PI);
    angle = fixmath::fixadd(angle, fixmath::itofix(16));
    angle &= (255<<16);
    angle >>= 16;
    angle /= 32;

    if (newCursor >= kSizeNCursor && newCursor <= kSizeNWCursor) {
      size_t num = sizeof(rotated_size_cursors) / sizeof(rotated_size_cursors[0]);
      size_t c;
      for (c=num-1; c>0; --c)
        if (rotated_size_cursors[c] == newCursor)
          break;

      newCursor = rotated_size_cursors[(c+angle) % num];
    }
    else if (newCursor >= kRotateNCursor && newCursor <= kRotateNWCursor) {
      size_t num = sizeof(rotated_rotate_cursors) / sizeof(rotated_rotate_cursors[0]);
      size_t c;
      for (c=num-1; c>0; --c)
        if (rotated_rotate_cursors[c] == newCursor)
          break;

      newCursor = rotated_rotate_cursors[(c+angle) % num];
    }

    editor->showMouseCursor(newCursor);
    return true;
  }

  SymmetryHandles handles;
  if (getSymmetryHandles(editor, handles)) {
    for (auto& handle : handles) {
      if (handle.rect().contains(mouseScreenPos)) {
        if (handle.axis() == Axis::HORIZONTAL)
          editor->showMouseCursor(kSizeWECursor);
        else
          editor->showMouseCursor(kSizeNSCursor);
        return true;
      }
    }
  }
  return false;
}

void StandbyState::Decorator::preRenderDecorator(EditorPreRender* render)
{
  // Do nothing
}

void StandbyState::Decorator::postRenderDecorator(EditorPostRender* render)
{
  Editor* editor = render->getEditor();

  // Draw transformation handles (if the mask is visible and isn't frozen).
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowMask &&
      editor->document()->isMaskVisible() &&
      !editor->document()->mask()->isFrozen()) {
    // And draw only when the user has a selection tool as active tool.
    tools::Ink* ink = editor->getCurrentEditorInk().get();

    if (ink->isSelection()) {
      getTransformHandles(editor)->drawHandles(editor,
        m_standbyState->getTransformation(editor));

      m_standbyState->m_transformSelectionHandlesAreVisible = true;
    }
  }

  // Draw transformation handles (if the mask is visible and isn't frozen).
  SymmetryHandles handles;
  if (StandbyState::Decorator::getSymmetryHandles(editor, handles)) {
    skin::SkinTheme* theme = static_cast<skin::SkinTheme*>(CurrentTheme::get());
    she::Surface* part = theme->parts.transformationHandle()->bitmap(0);
    ScreenGraphics g;
    for (auto& handle : handles)
      g.drawRgbaSurface(part, handle.rect().x, handle.rect().y);
  }
}

void StandbyState::Decorator::getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region)
{
  SymmetryHandles handles;
  if (getSymmetryHandles(editor, handles)) {
    for (auto& handle : handles)
      region.createUnion(region, gfx::Region(handle.rect()));
  }
}

bool StandbyState::Decorator::getSymmetryHandles(Editor* editor, SymmetryHandles& handles)
{
  // Draw transformation handles (if the mask is visible and isn't frozen).
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowSymmetryLine &&
      Preferences::instance().symmetryMode.enabled()) {
    const auto& symmetry = Preferences::instance().document(editor->document()).symmetry;
    auto mode = (int)symmetry.mode();

    if (mode == (int)app::gen::SymmetryMode::NONE)
      return false;

    gfx::Rect spriteBounds = editor->sprite()->bounds();
    gfx::Rect editorViewport = View::getView(editor)->viewportBounds();
    skin::SkinTheme* theme = static_cast<skin::SkinTheme*>(CurrentTheme::get());
    she::Surface* part = theme->parts.transformationHandle()->bitmap(0);
    gfx::Point pt1, pt2;

    if (mode & (int)app::gen::SymmetryMode::HORIZONTAL) {
      int pos = symmetry.xAxis();
      pt1 = gfx::Point(spriteBounds.x + pos, spriteBounds.y);
      pt1 = editor->editorToScreen(pt1);
      pt2 = gfx::Point(spriteBounds.x+ pos, spriteBounds.y+spriteBounds.h);
      pt2 = editor->editorToScreen(pt2);
      pt1.y = std::max(pt1.y-part->height(), editorViewport.y);
      pt2.y = std::min(pt2.y, editorViewport.point2().y-part->height());
      pt1.x -= part->width()/2;
      pt2.x -= part->width()/2;

      handles.push_back(
        SymmetryHandle(gfx::Rect(pt1.x, pt1.y, part->width(), part->height()), Axis::HORIZONTAL));
      handles.push_back(
        SymmetryHandle(gfx::Rect(pt2.x, pt2.y, part->width(), part->height()), Axis::HORIZONTAL));
    }

    if (mode & (int)app::gen::SymmetryMode::VERTICAL) {
      int pos = symmetry.yAxis();
      pt1 = gfx::Point(spriteBounds.x, spriteBounds.y + pos);
      pt1 = editor->editorToScreen(pt1);
      pt2 = gfx::Point(spriteBounds.x+spriteBounds.w, spriteBounds.y + pos);
      pt2 = editor->editorToScreen(pt2);
      pt1.x = std::max(pt1.x-part->width(), editorViewport.x);
      pt2.x = std::min(pt2.x, editorViewport.point2().x-part->width());
      pt1.y -= part->height()/2;
      pt2.y -= part->height()/2;

      handles.push_back(
        SymmetryHandle(gfx::Rect(pt1.x, pt1.y, part->width(), part->height()), Axis::VERTICAL));
      handles.push_back(
        SymmetryHandle(gfx::Rect(pt2.x, pt2.y, part->width(), part->height()), Axis::VERTICAL));
    }

    return true;
  }
  return false;
}

} // namespace app
