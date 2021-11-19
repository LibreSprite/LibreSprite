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
  void onExecute(Context* context) override;
};

static int _sprite_counter = 0;

NewFileCommand::NewFileCommand()
  : Command("NewFile",
            "New File",
            CmdRecordableFlag)
{
}

/**
 * Shows the "New Sprite" dialog.
 */
void NewFileCommand::onExecute(Context* context)
{
  Preferences& pref = Preferences::instance();
  int ncolors = get_default_palette()->size();
  char buf[1024];
  app::Color bg_table[] = {
    app::Color::fromMask(),
    app::Color::fromRgb(255, 255, 255),
    app::Color::fromRgb(0, 0, 0),
  };

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
    bool ok = false;

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

    // Select the color
    app::Color color = app::Color::fromMask();

    if (bg >= 0 && bg <= 3) {
      color = bg_table[bg];
      ok = true;
    }

    if (ok) {
      // Save the configuration
      pref.newFile.width(w);
      pref.newFile.height(h);
      pref.newFile.colorMode(format);
      pref.newFile.backgroundColor(bg);

      // Create the new sprite
      ASSERT(format == IMAGE_RGB || format == IMAGE_GRAYSCALE || format == IMAGE_INDEXED);
      ASSERT(w > 0 && h > 0);

      std::unique_ptr<Sprite> sprite(Sprite::createBasicSprite(format, w, h, ncolors));

      if (sprite->pixelFormat() != IMAGE_GRAYSCALE)
        get_default_palette()->copyColorsTo(sprite->palette(frame_t(0)));

      // If the background color isn't transparent, we have to
      // convert the `Layer 1' in a `Background'
      if (color.getType() != app::Color::MaskType) {
        Layer* layer = sprite->folder()->getFirstLayer();

        if (layer && layer->isImage()) {
          LayerImage* layerImage = static_cast<LayerImage*>(layer);
          layerImage->configureAsBackground();

          Image* image = layerImage->cel(frame_t(0))->image();

          // TODO Replace this adding a new parameter to color utils
          Palette oldPal = *get_current_palette();
          set_current_palette(get_default_palette(), false);

          doc::clear_image(image,
            color_utils::color_for_target(color,
              ColorTarget(
                ColorTarget::BackgroundLayer,
                sprite->pixelFormat(),
                sprite->transparentColor())));

          set_current_palette(&oldPal, false);
        }
      }

      // Show the sprite to the user
      std::unique_ptr<Document> doc(new Document(sprite.get()));
      sprite.release();
      sprintf(buf, "Sprite-%04d", ++_sprite_counter);
      doc->setFilename(buf);
      doc->setContext(context);
      doc.release();
    }
  }
}

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}

} // namespace app
