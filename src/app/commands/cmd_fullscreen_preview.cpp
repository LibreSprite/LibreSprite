// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/ui.h"

#include "app/app.h"
#include "app/app_render.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/status_bar.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "she/scoped_handle.h"
#include "she/surface.h"
#include "she/system.h"

#include <cstring>

#define PREVIEW_TILED           1
#define PREVIEW_FIT_ON_SCREEN   2

namespace app {

using namespace ui;
using namespace doc;
using namespace filters;

class PreviewWindow : public Window {
public:
  PreviewWindow(Context* context, Editor* editor)
    : Window(DesktopWindow)
    , m_context(context)
    , m_editor(editor)
    , m_doc(editor->document())
    , m_sprite(editor->sprite())
    , m_pal(m_sprite->palette(editor->frame()))
    , m_zoom(editor->zoom())
    , m_index_bg_color(-1)
    , m_doublebuf(Image::create(IMAGE_RGB, ui::display_w(), ui::display_h()))
    , m_doublesur(she::instance()->createRgbaSurface(ui::display_w(), ui::display_h())) {
    // Do not use DocumentWriter (do not lock the document) because we
    // will call other sub-commands (e.g. previous frame, next frame,
    // etc.).
    View* view = View::getView(editor);
    DocumentPreferences& docPref = Preferences::instance().document(m_doc);
    m_tiled = (filters::TiledMode)docPref.tiled.mode();

    // Free mouse
    editor->manager()->freeMouse();

    // Clear extras (e.g. pen preview)
    m_doc->setExtraCel(ExtraCelRef(nullptr));

    gfx::Rect vp = view->viewportBounds();
    gfx::Point scroll = view->viewScroll();

    m_oldMousePos = ui::get_mouse_position();
    m_pos.x = -scroll.x + vp.x + editor->padding().x;
    m_pos.y = -scroll.y + vp.y + editor->padding().y;

    setFocusStop(true);
    captureMouse();
  }

protected:
  virtual bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kCloseMessage:
        releaseMouse();
        break;

      case kMouseMoveMessage: {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point mousePos = mouseMsg->position();

        gfx::Rect bounds = this->bounds();
        gfx::Border border;
        if (bounds.w > 64*guiscale()) {
          border.left(32*guiscale());
          border.right(32*guiscale());
        }
        if (bounds.h > 64*guiscale()) {
          border.top(32*guiscale());
          border.bottom(32*guiscale());
        }

        m_delta += mousePos - m_oldMousePos;
        m_oldMousePos = mousePos;

        invalidate();
        break;
      }

      case kMouseUpMessage: {
        closeWindow(this);
        break;
      }

      case kKeyDownMessage: {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
        Command* command = NULL;
        Params params;
        KeyboardShortcuts::instance()
          ->getCommandFromKeyMessage(msg, &command, &params);

        // Change frame
        if (command != NULL &&
            (command->id() == CommandId::GotoFirstFrame ||
             command->id() == CommandId::GotoPreviousFrame ||
             command->id() == CommandId::GotoNextFrame ||
             command->id() == CommandId::GotoLastFrame)) {
          m_context->executeCommand(command, params);
          invalidate();
          m_render.reset(NULL); // Re-render
        }
#if 0
        // Play the animation
        else if (command != NULL &&
                 std::strcmp(command->short_name(), CommandId::PlayAnimation) == 0) {
          // TODO
        }
#endif
        // Change background color
        else if (keyMsg->scancode() == kKeyPlusPad ||
                 keyMsg->unicodeChar() == '+') {
          if (m_index_bg_color == -1 ||
            m_index_bg_color < m_pal->size()-1) {
            ++m_index_bg_color;

            invalidate();
          }
        }
        else if (keyMsg->scancode() == kKeyMinusPad ||
                 keyMsg->unicodeChar() == '-') {
          if (m_index_bg_color >= 0) {
            --m_index_bg_color;     // can be -1 which is the checked background

            invalidate();
          }
        }
        else {
          closeWindow(this);
        }

        return true;
      }

      case kSetCursorMessage:
        ui::set_mouse_cursor(kNoCursor);
        return true;
    }

    return Window::onProcessMessage(msg);
  }

  virtual void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    AppRender& render = m_editor->renderEngine();
    render.disableOnionskin();
    render.setBgType(render::BgType::TRANSPARENT);

    // Render sprite and leave the result in 'm_render' variable
    if (m_render == NULL) {
      ImageBufferPtr buf = Editor::getRenderImageBuffer();
      m_render.reset(Image::create(IMAGE_RGB,
          m_sprite->width(), m_sprite->height(), buf));

      render.renderSprite(
        m_render.get(), m_sprite, m_editor->frame());
    }

    int x, y, w, h, u, v;
    x = m_pos.x + m_zoom.apply(m_zoom.remove(m_delta.x));
    y = m_pos.y + m_zoom.apply(m_zoom.remove(m_delta.y));
    w = m_zoom.apply(m_sprite->width());
    h = m_zoom.apply(m_sprite->height());

    if (int(m_tiled) & int(TiledMode::X_AXIS)) x = SGN(x) * (ABS(x)%w);
    if (int(m_tiled) & int(TiledMode::Y_AXIS)) y = SGN(y) * (ABS(y)%h);

    if (m_index_bg_color == -1) {
      render.setupBackground(m_doc, m_doublebuf->pixelFormat());
      render.renderBackground(m_doublebuf.get(),
        gfx::Clip(0, 0, -m_pos.x, -m_pos.y,
          m_doublebuf->width(), m_doublebuf->height()), m_zoom);
    }
    else {
      doc::clear_image(m_doublebuf.get(), m_pal->getEntry(m_index_bg_color));
    }

    switch (m_tiled) {
      case TiledMode::NONE:
        render.renderImage(m_doublebuf.get(), m_render.get(), m_pal, x, y,
                           m_zoom, 255, BlendMode::NORMAL);
        break;
      case TiledMode::X_AXIS:
        for (u=x-w; u<ui::display_w()+w; u+=w)
          render.renderImage(m_doublebuf.get(), m_render.get(), m_pal, u, y,
                             m_zoom, 255, BlendMode::NORMAL);
        break;
      case TiledMode::Y_AXIS:
        for (v=y-h; v<ui::display_h()+h; v+=h)
          render.renderImage(m_doublebuf.get(), m_render.get(), m_pal, x, v,
                             m_zoom, 255, BlendMode::NORMAL);
        break;
      case TiledMode::BOTH:
        for (v=y-h; v<ui::display_h()+h; v+=h)
          for (u=x-w; u<ui::display_w()+w; u+=w)
            render.renderImage(m_doublebuf.get(), m_render.get(), m_pal, u, v,
                               m_zoom, 255, BlendMode::NORMAL);
        break;
    }

    doc::convert_image_to_surface(m_doublebuf.get(), m_pal,
      m_doublesur, 0, 0, 0, 0, m_doublebuf->width(), m_doublebuf->height());
    g->blit(m_doublesur, 0, 0, 0, 0, m_doublesur->width(), m_doublesur->height());
  }

private:
  Context* m_context;
  Editor* m_editor;
  Document* m_doc;
  Sprite* m_sprite;
  const Palette* m_pal;
  gfx::Point m_pos;
  gfx::Point m_oldMousePos;
  gfx::Point m_delta;
  render::Zoom m_zoom;
  int m_index_bg_color;
  std::unique_ptr<Image> m_render;
  std::unique_ptr<Image> m_doublebuf;
  she::ScopedHandle<she::Surface> m_doublesur;
  filters::TiledMode m_tiled;
};

class FullscreenPreviewCommand : public Command {
public:
  FullscreenPreviewCommand();
  Command* clone() const override { return new FullscreenPreviewCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

FullscreenPreviewCommand::FullscreenPreviewCommand()
  : Command("FullscreenPreview",
            "Fullscreen Preview",
            CmdUIOnlyFlag)
{
}

bool FullscreenPreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

// Shows the sprite using the complete screen.
void FullscreenPreviewCommand::onExecute(Context* context)
{
  Editor* editor = current_editor;

  // Cancel operation if current editor does not have a sprite
  if (!editor || !editor->sprite())
    return;

  PreviewWindow window(context, editor);
  window.openWindowInForeground();
}

Command* CommandFactory::createFullscreenPreviewCommand()
{
  return new FullscreenPreviewCommand;
}

} // namespace app
