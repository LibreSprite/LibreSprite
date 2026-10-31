// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C)      2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/font_popup.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/freetype_utils.h"
#include "base/bind.h"
#include "base/log.h"
#include "base/path.h"
#include "base/string.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "she/system.h"
#include "she/font.h"
#include "paste_text.xml.h"

#include <memory>

namespace app {

static std::string last_text_used;

class PasteTextCommand : public Command {
public:
  PasteTextCommand();
  Command* clone() const override { return new PasteTextCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;

private:
  // Renders the text and converts it to the sprite's pixel format (shared
  // by the dialog and param-based paths).
  doc::ImageRef renderTextImage(const std::string& faceName, int size,
                                const std::string& text, const app::Color& color,
                                bool antialias, Sprite* sprite, frame_t frame) const;

  // Param-based path: stamps the rendered text in the active cel. It is
  // headless-safe, unlike the editor's interactive paste preview.
  void stampText(Context* ctx, const std::string& faceName, int size,
                 const std::string& text, const app::Color& color,
                 bool antialias) const;

  std::string m_text;   // from the text param
  std::string m_font;   // from the font param
  int m_size;           // 0 = not specified
  int m_antialias;      // -1 = not specified, else 0/1
  std::string m_color;  // from the color param
};

// Shows a render error (an alert on the GUI, a console hint headless).
static void showRenderError(const std::exception& ex) {
  // The alert would block headless.
  if (App::instance()->isGui())
    ui::Alert::show(PACKAGE
                    "<<%s"
                    "||&OK", ex.what());
  else
    Console::showException(ex);
}

PasteTextCommand::PasteTextCommand()
  : Command("PasteText",
            "Insert Text",
            CmdUIOnlyFlag)
{
  m_size = 0;
  m_antialias = -1;
}

void PasteTextCommand::onLoadParams(const Params& params)
{
  m_text = params.get("text");
  m_font = params.get("font");
  m_size = 0;
  m_antialias = -1;
  if (params.has_param("size"))
    m_size = params.get_as<int>("size");
  std::string antialias = params.get("antialias");
  if (antialias == "true")
    m_antialias = 1;
  else if (antialias == "false")
    m_antialias = 0;
  m_color = params.get("color");
}

std::vector<CommandParamInfo> PasteTextCommand::onDescribeParams() const
{
  return {
    {"text", "Text to insert. With font, inserts without the dialog.", false, ""},
    {"font", "Path to a .ttf or .otf font file. With text, inserts without the dialog.", false, ""},
    {"size", "Font size in points. Uses the text tool preference when empty.", false, ""},
    {"antialias", "Antialiasing: true or false. Uses the text tool preference when empty.", false, ""},
    {"color", "Text color, for example rgb{255,0,0,255}. Uses the foreground color when empty.", false, ""},
  };
}

bool PasteTextCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::ActiveLayerIsEditable);
}

class PasteTextWindow : public app::gen::PasteText {
public:
  PasteTextWindow(const std::string& face, int size,
                  bool antialias,
                  const app::Color& color) {
    this->antialias()->setSelected(antialias);
    if (!face.empty())
      setFontFace(face);
    ok()->setEnabled(!m_face.empty());
    if (!m_face.empty())
      updateFontFaceButton();

    fontSize()->setTextf("%d", size);
    fontFace()->Click.connect(base::Bind<void>(&PasteTextWindow::onSelectFontFile, this));
    fontFace()->DropDownClick.connect(base::Bind<void>(&PasteTextWindow::onSelectSystemFont, this));
    fontColor()->setColor(color);
    this->antialias()->Click.connect([this](ui::Event&){onChangeAntialias();});
  }

  std::string faceValue() const {
    return m_face;
  }

  int sizeValue() const {
    int size = fontSize()->textInt();
    size = MID(1, size, 5000);
    return size;
  }

private:
  void updateFontFaceButton() {
    fontFace()->mainButton()
      ->setTextf("Select Font: %s",
                 base::get_file_title(m_face).c_str());
  }

  void onSelectFontFile() {
    std::string face = show_file_selector(
      "Select a TrueType Font",
      m_face,
      "ttf,otf",
      FileSelectorType::Open,
      nullptr);

    if (!face.empty()) {
      setFontFace(face);
    }
  }

  void setFontFace(const std::string& face) {
    if (face == m_face)
      return;

    m_face = face;
    ok()->setEnabled(true);
    updateFontFaceButton();

    m_font.reset(she::instance()->loadTrueTypeFont(face.c_str(), 12));
    if (m_font)
      m_font->setAntialias(antialias()->isSelected());

    userText()->resetFont(m_font);
    userText()->invalidate();
  }

  void onChangeAntialias() {
    if (m_font) {
      m_font->setAntialias(antialias()->isSelected());
      userText()->invalidate();
    }
  }

  void onSelectSystemFont() {
    if (!m_fontPopup) {
      try {
        m_fontPopup.reset(new FontPopup());
        m_fontPopup->Load.connect(&PasteTextWindow::setFontFace, this);
        m_fontPopup->Close.connect(base::Bind<void>(&PasteTextWindow::onCloseFontPopup, this));
      }
      catch (const std::exception& ex) {
        Console::showException(ex);
        return;
      }
    }

    if (!m_fontPopup->isVisible()) {
      gfx::Rect bounds = fontFace()->bounds();
      m_fontPopup->showPopup(
        gfx::Rect(bounds.x, bounds.y+bounds.h,
                  ui::display_w()/2, ui::display_h()/2));
    }
    else {
      m_fontPopup->closeWindow(NULL);
    }
  }

  void onCloseFontPopup() {
    fontFace()->dropDown()->requestFocus();
  }

  std::shared_ptr<she::Font> m_font;
  std::string m_face;
  std::unique_ptr<FontPopup> m_fontPopup;
};

doc::ImageRef PasteTextCommand::renderTextImage(const std::string& faceName, int size,
                                                const std::string& text, const app::Color& appColor,
                                                bool antialias, Sprite* sprite,
                                                frame_t frame) const
{
  doc::color_t color = doc::rgba(appColor.getRed(),
                                 appColor.getGreen(),
                                 appColor.getBlue(),
                                 appColor.getAlpha());

  doc::ImageRef image(render_text(faceName, size, text, color, antialias));
  if (image && image->pixelFormat() != sprite->pixelFormat()) {
    RgbMap* rgbmap = sprite->rgbMap(frame);
    image.reset(
      render::convert_pixel_format(
        image.get(), NULL, sprite->pixelFormat(),
        DitheringMethod::NONE, rgbmap, sprite->palette(frame),
        false, 0));
  }
  return image;
}

void PasteTextCommand::stampText(Context* ctx, const std::string& faceName,
                                 int size, const std::string& text,
                                 const app::Color& appColor,
                                 bool antialias) const
{
  try {
    ContextWriter writer(ctx);
    Sprite* sprite = writer.sprite();
    if (!sprite)
      return;

    doc::ImageRef image = renderTextImage(faceName, size, text, appColor,
                                          antialias, sprite, writer.frame());
    if (!image)
      return;

    // Center the text in the sprite (the editor's visible bounds have no
    // meaning headless).
    int x = (sprite->width() - image->width()) / 2;
    int y = (sprite->height() - image->height()) / 2;
    x = MID(0, x, sprite->width());
    y = MID(0, y, sprite->height());

    // The write region, clipped to the sprite bounds.
    gfx::Rect writeRect(x, y, image->width(), image->height());
    writeRect = writeRect.createIntersection(
      gfx::Rect(0, 0, sprite->width(), sprite->height()));
    if (writeRect.isEmpty())
      return;

    Transaction transaction(ctx, "Insert Text", ModifyDocument);
    doc::Site site = *writer.site();
    ExpandCelCanvas ecs(site, writer.layer(),
                        filters::TiledMode::NONE, transaction,
                        ExpandCelCanvas::NeedsSource);
    ecs.validateDestCanvas(gfx::Region(writeRect));

    doc::Image* dst = ecs.getDestCanvas();
    const gfx::Point origin(ecs.getCel()->x(), ecs.getCel()->y());
    dst->copy(image.get(),
              gfx::Clip(writeRect.x - origin.x, writeRect.y - origin.y,
                        writeRect.x - x, writeRect.y - y,
                        writeRect.w, writeRect.h));

    ecs.commit();
    transaction.commit();
  }
  catch (const std::exception& ex) {
    showRenderError(ex);
  }
}

void PasteTextCommand::onExecute(Context* ctx)
{
  Editor* editor = current_editor;
  if (editor == NULL)
    return;

  Preferences& pref = Preferences::instance();

  // Param-based: text and font are given, so the dialog is skipped.
  // Missing options use the text tool preferences, like the dialog defaults.
  if (!m_text.empty() && !m_font.empty()) {
    int size = (m_size != 0 ? MID(1, m_size, 999)
                            : MID(1, pref.textTool.fontSize(), 999));
    bool antialias = (m_antialias >= 0 ? m_antialias != 0
                                       : pref.textTool.antialias());
    app::Color color = (!m_color.empty() ? app::Color::fromString(m_color)
                                         : pref.colorBar.fgColor());
    stampText(ctx, m_font, size, m_text, color, antialias);
    return;
  }

  // The dialog would block headless.
  if (!App::instance()->isGui()) {
    LOG("PasteText: text and font params are required in headless mode\n");
    Console console(ctx);
    console.printf("PasteText: text and font params are required in headless mode\n");
    return;
  }

  PasteTextWindow window(pref.textTool.fontFace(),
                         pref.textTool.fontSize(),
                         pref.textTool.antialias(),
                         pref.colorBar.fgColor());

  window.userText()->setText(last_text_used);

  window.openWindowInForeground();
  if (window.closer() != window.ok())
    return;

  last_text_used = window.userText()->text();

  bool antialias = window.antialias()->isSelected();
  std::string faceName = window.faceValue();
  int size = window.sizeValue();
  size = MID(1, size, 999);
  // Only the dialog path persists the text tool preferences.
  pref.textTool.fontFace(faceName);
  pref.textTool.fontSize(size);
  pref.textTool.antialias(antialias);

  app::Color appColor = window.fontColor()->getColor();
  try {
    Sprite* sprite = editor->sprite();
    doc::ImageRef image = renderTextImage(faceName, size,
                                          window.userText()->text(),
                                          appColor, antialias,
                                          sprite, editor->frame());
    if (image)
      editor->pasteImage(image.get());
  }
  catch (const std::exception& ex) {
    showRenderError(ex);
  }
}

Command* CommandFactory::createPasteTextCommand()
{
  return new PasteTextCommand;
}

} // namespace app
