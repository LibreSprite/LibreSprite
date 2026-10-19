// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/button_set.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "new_sprite.xml.h"

#include <memory>

using namespace ui;

namespace app {

class NewFileCommand : public Command {
public:
  NewFileCommand();
  Command* clone() const override { return new NewFileCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  void onExecute(Context* context) override;

private:
  // Creates the sprite and opens it as a new document (shared by the
  // dialog and param-based paths).
  void createSprite(Context* context, PixelFormat format, int w, int h, int bg) const;

  int m_width;      // 0 = not specified
  int m_height;     // 0 = not specified
  int m_format;     // -1 = not specified, else a PixelFormat
  int m_background; // -1 = not specified, else 0=mask, 1=white, 2=black
};

static int _sprite_counter = 0;

NewFileCommand::NewFileCommand()
  : Command("NewFile",
            "New File",
            CmdRecordableFlag)
{
  m_width = 0;
  m_height = 0;
  m_format = -1;
  m_background = -1;
}

void NewFileCommand::onLoadParams(const Params& params)
{
  std::string width = params.get("width");
  m_width = (width.empty() ? 0 : std::strtol(width.c_str(), NULL, 10));

  std::string height = params.get("height");
  m_height = (height.empty() ? 0 : std::strtol(height.c_str(), NULL, 10));

  std::string color_mode = params.get("color-mode");
  if (color_mode == "rgb")
    m_format = IMAGE_RGB;
  else if (color_mode == "grayscale")
    m_format = IMAGE_GRAYSCALE;
  else if (color_mode == "indexed")
    m_format = IMAGE_INDEXED;
  else
    m_format = -1;

  std::string background = params.get("background");
  if (background == "transparent")
    m_background = 0;
  else if (background == "white")
    m_background = 1;
  else if (background == "black")
    m_background = 2;
  else
    m_background = -1;
}

std::vector<CommandParamInfo> NewFileCommand::onDescribeParams() const
{
  return {
    {"width", "Sprite width in pixels. With height, creates the sprite without the dialog.", false, ""},
    {"height", "Sprite height in pixels. With width, creates the sprite without the dialog.", false, ""},
    {"color-mode", "Pixel format: rgb, indexed or grayscale. Uses the New File preference when empty.", false, ""},
    {"background", "Background color: transparent, white or black. Uses the New File preference when empty.", false, ""},
  };
}

void NewFileCommand::createSprite(Context* context, PixelFormat format, int w, int h, int bg) const
{
  int ncolors = get_default_palette()->size();
  char buf[1024];
  app::Color bg_table[] = {
    app::Color::fromMask(),
    app::Color::fromRgb(255, 255, 255),
    app::Color::fromRgb(0, 0, 0),
  };
  app::Color color = bg_table[bg];

  // Create the new sprite
  ASSERT(format == IMAGE_RGB || format == IMAGE_GRAYSCALE || format == IMAGE_INDEXED);
  ASSERT(w > 0 && h > 0);

  std::unique_ptr<Sprite> sprite(Sprite::createBasicSprite(format, w, h, ncolors));

  if (sprite->pixelFormat() != IMAGE_GRAYSCALE)
    get_default_palette()->copyColorsTo(*sprite->palette(frame_t(0)));

  // If the background color isn't transparent, we have to
  // convert the `Layer 1' in a `Background'
  if (color.getType() != app::Color::MaskType) {
    Layer* layer = sprite->folder()->getFirstLayer();

    if (layer && layer->isImage()) {
      LayerImage* layerImage = static_cast<LayerImage*>(layer);
      layerImage->configureAsBackground();

      Image* image = layerImage->cel(frame_t(0))->image();

      // TODO Replace this adding a new parameter to color utils
      auto oldPal = get_current_palette()->clone();
      set_current_palette(get_default_palette(), false);

      doc::clear_image(image,
        color_utils::color_for_target(color,
          ColorTarget(
            ColorTarget::BackgroundLayer,
            sprite->pixelFormat(),
            sprite->transparentColor())));

      set_current_palette(oldPal.get(), false);
    }
  }

  // Show the sprite to the user
  std::unique_ptr<Document> doc(new Document(sprite.get()));
  sprite.release();
  snprintf(buf, sizeof(buf), "Sprite-%04d", ++_sprite_counter);
  doc->setFilename(buf);
  doc->setContext(context);
  doc.release();
}

/**
 * Shows the "New Sprite" dialog, or creates the sprite directly when the
 * width and height params are given (e.g. from a script).
 */
void NewFileCommand::onExecute(Context* context)
{
  Preferences& pref = Preferences::instance();

  // Param-based creation: both width and height are given, so the dialog
  // is skipped. Missing options use the New File preferences, like the
  // dialog defaults.
  if (m_width != 0 && m_height != 0) {
    PixelFormat format = (m_format >= 0 ? (PixelFormat)m_format
                                        : pref.newFile.colorMode());
    // Invalid format in config file.
    if (format != IMAGE_RGB &&
        format != IMAGE_GRAYSCALE &&
        format != IMAGE_INDEXED) {
      format = IMAGE_INDEXED;
    }
    int bg = (m_background >= 0 ? m_background : pref.newFile.backgroundColor());

    createSprite(context, format,
                 MID(1, m_width, 65535), MID(1, m_height, 65535), MID(0, bg, 2));
    return;
  }

  // Load the window widget
  app::gen::NewSprite window;

  // Default values: Indexed, 320x240, Background color
  PixelFormat format = pref.newFile.colorMode();
  // Invalid format in config file.
  if (format != IMAGE_RGB &&
      format != IMAGE_INDEXED &&
      format != IMAGE_GRAYSCALE) {
    format = IMAGE_INDEXED;
  }
  int w = pref.newFile.width();
  int h = pref.newFile.height();
  int bg = pref.newFile.backgroundColor();
  bg = MID(0, bg, 2);

  // If the clipboard contains an image, we can show the size of the
  // clipboard as default image size.
  gfx::Size clipboardSize;
  if (clipboard::get_image_size(clipboardSize)) {
    w = clipboardSize.w;
    h = clipboardSize.h;
  }

  window.width()->setTextf("%d", MAX(1, w));
  window.height()->setTextf("%d", MAX(1, h));

  // Select image-type
  window.colorMode()->setSelectedItem(format);

  // Select background color
  window.bgColor()->setSelectedItem(bg);

  // Open the window
  window.openWindowInForeground();

  if (window.closer() == window.okButton()) {
    // Get the options
    format = (doc::PixelFormat)window.colorMode()->selectedItem();
    w = window.width()->textInt();
    h = window.height()->textInt();
    bg = window.bgColor()->selectedItem();

    static_assert(IMAGE_RGB == 0, "RGB pixel format should be 0");
    static_assert(IMAGE_INDEXED == 2, "Indexed pixel format should be 2");

    format = MID(IMAGE_RGB, format, IMAGE_INDEXED);
    w = MID(1, w, 65535);
    h = MID(1, h, 65535);
    bg = MID(0, bg, 2);

    // Save the configuration
    pref.newFile.width(w);
    pref.newFile.height(h);
    pref.newFile.colorMode(format);
    pref.newFile.backgroundColor(bg);

    createSprite(context, format, w, h, bg);
  }
}

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}

} // namespace app
