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

#include <cstdlib>
#include <memory>

using namespace ui;

#include <cmath>
#include <cctype>
#include <stdexcept>
#include <algorithm>

class ExprParser {
  std::string src;
  std::size_t pos;
  double baseValue;
  bool hasError;
  std::string errorMsg;

  void skipWhitespace() {
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) {
      pos++;
    }
  }

  bool matchStr(const std::string& s) {
    if (pos + s.size() > src.size()) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(src[pos + i])) != std::tolower(static_cast<unsigned char>(s[i]))) {
        return false;
      }
    }
    pos += s.size();
    return true;
  }

public:
  ExprParser(const std::string& input, double baseVal)
      : src(input), pos(0), baseValue(baseVal), hasError(false) {
    skipWhitespace();
    if (pos < src.size()) {
      char c = src[pos];
      if (c == '+' || c == '-' || c == '*' || c == '/') {
        src = std::to_string(baseValue) + src.substr(pos);
        pos = 0;
      }
    }
  }

  double parseExpression() {
    double val = parseTerm();
    while (true) {
      skipWhitespace();
      if (pos >= src.size()) break;
      char op = src[pos];
      if (op == '+' || op == '-') {
        pos++;
        double rhs = parseTerm();
        if (op == '+') val += rhs;
        else val -= rhs;
      } else {
        break;
      }
    }
    return val;
  }

  double parseTerm() {
    double val = parseFactor();
    while (true) {
      skipWhitespace();
      if (pos >= src.size()) break;
      char op = src[pos];
      if (op == '*' || op == '/') {
        pos++;
        double rhs = parseFactor();
        if (op == '*') {
          val *= rhs;
        } else {
          if (rhs == 0.0) {
            hasError = true;
            errorMsg = "Division by zero";
            val = 0.0;
          } else {
            val /= rhs;
          }
        }
      } else {
        break;
      }
    }
    return val;
  }

  double parseFactor() {
    skipWhitespace();
    if (pos >= src.size()) {
      hasError = true;
      errorMsg = "Unexpected end of expression";
      return 0.0;
    }

    bool negative = false;
    if (src[pos] == '+') {
      pos++;
      skipWhitespace();
    } else if (src[pos] == '-') {
      pos++;
      negative = true;
      skipWhitespace();
    }

    double val = 0.0;
    if (src[pos] == '(') {
      pos++;
      val = parseExpression();
      skipWhitespace();
      if (pos < src.size() && src[pos] == ')') {
        pos++;
      } else {
        hasError = true;
        errorMsg = "Missing closing parenthesis";
      }
    } else if (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.') {
      std::size_t nextPos = 0;
      try {
        val = std::stod(src.substr(pos), &nextPos);
        pos += nextPos;
      } catch (...) {
        hasError = true;
        errorMsg = "Invalid number format";
      }
    } else {
      hasError = true;
      errorMsg = std::string("Unexpected character: '") + src[pos] + "'";
      pos++;
    }

    if (negative) {
      val = -val;
    }

    while (true) {
      skipWhitespace();
      if (pos >= src.size()) break;
      if (src[pos] == '%') {
        val *= 0.01;
        pos++;
      } else if (matchStr("px")) {
        continue;
      } else {
        break;
      }
    }

    return val;
  }

  bool getResult(double& outResult) {
    try {
      outResult = parseExpression();
      skipWhitespace();
      if (pos < src.size()) {
        if (matchStr("px")) {
          skipWhitespace();
        }
        if (pos < src.size()) {
          hasError = true;
          errorMsg = std::string("Unexpected trailing characters: '") + src.substr(pos) + "'";
        }
      }
    } catch (const std::exception& e) {
      hasError = true;
      errorMsg = e.what();
    }
    return !hasError;
  }

  std::string getError() const { return errorMsg; }
};

// Evaluate an arithmetic expression relative to a base value.
// Supports binary expressions (e.g. 16*5, 100+50, 200/2) and
// prefix operators (+N, -N, *N, /N — relative to base).
// Trailing suffixes like "px" or "%" are stripped automatically.
static int evalExpr(const std::string& rawText, int base, bool reportError = false) {
  double result = 0.0;
  ExprParser parser(rawText, base);
  if (parser.getResult(result)) {
    return (int)std::round(result);
  } else {
    if (reportError) {
      app::Console console;
      console.printf("Error evaluating expression '%s': %s\n", rawText.c_str(), parser.getError().c_str());
    }
    return base;
  }
}

// Returns true if 'rawText' contains an arithmetic operator (binary or prefix).
static bool isExpr(const std::string& rawText) {
  for (char c : rawText) {
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '(' || c == ')' || c == '.') {
      return true;
    }
  }
  return false;
}


namespace app {

class NewSpriteWindow : public app::gen::NewSprite {
public:
  NewSpriteWindow(int defaultWidth, int defaultHeight)
    : m_baseWidth(defaultWidth)
    , m_baseHeight(defaultHeight) {
  }

  ~NewSpriteWindow() {
    if (m_filterManager)
      m_filterManager->removeMessageFilterFor(this);
  }

  int getWidth()  const { return evalExpr(width()->text(),  m_baseWidth, true); }
  int getHeight() const { return evalExpr(height()->text(), m_baseHeight, true); }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kOpenMessage:
        m_filterManager = manager();
        m_filterManager->addMessageFilter(kKeyDownMessage, this);
        m_filterManager->addMessageFilter(kFocusLeaveMessage, this);
        break;
      case kKeyDownMessage: {
        auto sc = static_cast<KeyMessage*>(msg)->scancode();
        if (sc == kKeyTab || sc == kKeyEnter) {
          Widget* f = manager()->getFocus();
          if (f == width() && isExpr(width()->text())) {
            width()->setTextf("%d", std::max(1, evalExpr(width()->text(), m_baseWidth, true)));
          }
          else if (f == height() && isExpr(height()->text())) {
            height()->setTextf("%d", std::max(1, evalExpr(height()->text(), m_baseHeight, true)));
          }
        }
        break;
      }
      case kFocusLeaveMessage: {
        Manager* mgr = manager();
        if (!mgr) break;
        Widget* cur = mgr->getFocus();
        if (cur != width() && isExpr(width()->text())) {
          width()->setTextf("%d", std::max(1, evalExpr(width()->text(), m_baseWidth, true)));
        }
        if (cur != height() && isExpr(height()->text())) {
          height()->setTextf("%d", std::max(1, evalExpr(height()->text(), m_baseHeight, true)));
        }
        break;
      }
    }
    return Window::onProcessMessage(msg);
  }

private:
  int m_baseWidth;
  int m_baseHeight;
  Manager* m_filterManager = nullptr;
};

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

  // Load the window widget using our custom NewSpriteWindow class
  NewSpriteWindow window(w, h);

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
    w = window.getWidth();
    h = window.getHeight();
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
  }
}

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}

} // namespace app
