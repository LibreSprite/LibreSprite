// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_sprite_size.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/ini_file.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "base/bind.h"
#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "sprite_size.xml.h"

#include <cstdlib>
#include <memory>

#include "app/console.h"
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
#define PERC_FORMAT     "%.1f"

namespace app {

using namespace ui;
using doc::algorithm::ResizeMethod;

class SpriteSizeJob : public Job {
  ContextWriter m_writer;
  Document* m_document;
  Sprite* m_sprite;
  int m_new_width;
  int m_new_height;
  ResizeMethod m_resize_method;

  int scale_x(int x) const { return x * m_new_width / m_sprite->width(); }
  int scale_y(int y) const { return y * m_new_height / m_sprite->height(); }

public:

  SpriteSizeJob(const ContextReader& reader, int new_width, int new_height, ResizeMethod resize_method)
    : Job("Sprite Size")
    , m_writer(reader)
    , m_document(m_writer.document())
    , m_sprite(m_writer.sprite())
  {
    m_new_width = new_width;
    m_new_height = new_height;
    m_resize_method = resize_method;
  }

protected:

  /**
   * [working thread]
   */
  virtual void onJob()
  {
    Transaction transaction(m_writer.context(), "Sprite Size");
    DocumentApi api = m_writer.document()->getApi(transaction);

    int cels_count = 0;
    for (auto cel : m_sprite->uniqueCels()) { // TODO add size() member function to CelsRange
      (void)cel;
      ++cels_count;
    }

    // For each cel...
    int progress = 0;
    for (auto cel : m_sprite->uniqueCels()) {
      // Change its location
      api.setCelPosition(m_sprite, cel, scale_x(cel->x()), scale_y(cel->y()));

      // Get cel's image
      Image* image = cel->image();
      if (image && !cel->link()) {
        // Resize the image
        int w = scale_x(image->width());
        int h = scale_y(image->height());
        ImageRef new_image(Image::create(image->pixelFormat(), MAX(1, w), MAX(1, h)));
        new_image->setMaskColor(image->maskColor());

        doc::algorithm::fixup_image_transparent_colors(image);
        doc::algorithm::resize_image(
          image, new_image.get(),
          m_resize_method,
          m_sprite->palette(cel->frame()),
          m_sprite->rgbMap(cel->frame()),
          (cel->layer()->isBackground() ? -1: m_sprite->transparentColor()));

        api.replaceImage(m_sprite, cel->imageRef(), new_image);
      }

      jobProgress((float)progress / cels_count);
      ++progress;

      // cancel all the operation?
      if (isCanceled())
        return;        // Transaction destructor will undo all operations
    }

    // Resize mask
    if (m_document->isMaskVisible()) {
      ImageRef old_bitmap
        (crop_image(m_document->mask()->bitmap(), -1, -1,
                    m_document->mask()->bitmap()->width()+2,
                    m_document->mask()->bitmap()->height()+2, 0));

      int w = scale_x(old_bitmap->width());
      int h = scale_y(old_bitmap->height());
      std::unique_ptr<Mask> new_mask(new Mask);
      new_mask->replace(
        gfx::Rect(
          scale_x(m_document->mask()->bounds().x-1),
          scale_y(m_document->mask()->bounds().y-1), MAX(1, w), MAX(1, h)));
      algorithm::resize_image(
        old_bitmap.get(), new_mask->bitmap(),
        m_resize_method,
        m_sprite->palette(0), // Ignored
        m_sprite->rgbMap(0),  // Ignored
        -1);                  // Ignored

      // Reshrink
      new_mask->intersect(new_mask->bounds());

      // Copy new mask
      api.copyToCurrentMask(new_mask.get());

      // Regenerate mask
      m_document->resetTransformation();
      m_document->generateMaskBoundaries();
    }

    // resize sprite
    api.setSpriteSize(m_sprite, m_new_width, m_new_height);

    // commit changes
    transaction.commit();
  }

};

class SpriteSizeWindow : public app::gen::SpriteSize {
public:
  SpriteSizeWindow(Context* ctx, int new_width, int new_height)
    : m_ctx(ctx)
    , m_baseWidth(new_width)
    , m_baseHeight(new_height) {
    lockRatio()->Click.connect(base::Bind<void>(&SpriteSizeWindow::onLockRatioClick, this));
    widthPx()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onWidthPxChange, this));
    heightPx()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onHeightPxChange, this));
    widthPerc()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onWidthPercChange, this));
    heightPerc()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onHeightPercChange, this));

    widthPx()->setTextf("%d", new_width);
    heightPx()->setTextf("%d", new_height);

    static_assert(doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR == 0 &&
                  doc::algorithm::RESIZE_METHOD_BILINEAR == 1 &&
                  doc::algorithm::RESIZE_METHOD_ROTSPRITE == 2,
                  "ResizeMethod enum has changed");
    method()->addItem("Nearest-neighbor");
    method()->addItem("Bilinear");
    method()->addItem("RotSprite");
    method()->setSelectedItemIndex(
      get_config_int("SpriteSize", "Method",
                     doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR));
  }

  ~SpriteSizeWindow() {
    if (m_filterManager)
      m_filterManager->removeMessageFilterFor(this);
  }

  // Return the evaluated pixel width/height (handles expressions like *2, +50).
  int evalWidthPx()  const { return evalExpr(widthPx()->text(),  m_baseWidth, true); }
  int evalHeightPx() const { return evalExpr(heightPx()->text(), m_baseHeight, true); }

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
          if (f == widthPx() && isExpr(widthPx()->text())) {
            widthPx()->setTextf("%d", std::max(1, evalExpr(widthPx()->text(), m_baseWidth, true)));
            onWidthPxChange();
          }
          else if (f == heightPx() && isExpr(heightPx()->text())) {
            heightPx()->setTextf("%d", std::max(1, evalExpr(heightPx()->text(), m_baseHeight, true)));
            onHeightPxChange();
          }
        }
        break;
      }
      case kFocusLeaveMessage: {
        Manager* mgr = manager();
        if (!mgr) break;
        Widget* cur = mgr->getFocus();
        if (cur != widthPx() && isExpr(widthPx()->text())) {
          widthPx()->setTextf("%d", std::max(1, evalExpr(widthPx()->text(), m_baseWidth, true)));
          onWidthPxChange();
        }
        if (cur != heightPx() && isExpr(heightPx()->text())) {
          heightPx()->setTextf("%d", std::max(1, evalExpr(heightPx()->text(), m_baseHeight, true)));
          onHeightPxChange();
        }
        break;
      }
    }
    return Window::onProcessMessage(msg);
  }

private:

  void onLockRatioClick() {
    const ContextReader reader(m_ctx);
    onWidthPxChange();
  }

  void onWidthPxChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    int width = evalExpr(widthPx()->text(), m_baseWidth);
    double perc = 100.0 * width / sprite->width();

    widthPerc()->setTextf(PERC_FORMAT, perc);

    if (lockRatio()->isSelected()) {
      heightPerc()->setTextf(PERC_FORMAT, perc);
      heightPx()->setTextf("%d", sprite->height() * width / sprite->width());
    }
  }

  void onHeightPxChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    int height = evalExpr(heightPx()->text(), m_baseHeight);
    double perc = 100.0 * height / sprite->height();

    heightPerc()->setTextf(PERC_FORMAT, perc);

    if (lockRatio()->isSelected()) {
      widthPerc()->setTextf(PERC_FORMAT, perc);
      widthPx()->setTextf("%d", sprite->width() * height / sprite->height());
    }
  }

  void onWidthPercChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    double width = widthPerc()->textDouble();

    widthPx()->setTextf("%d", (int)(sprite->width() * width / 100));

    if (lockRatio()->isSelected()) {
      heightPx()->setTextf("%d", (int)(sprite->height() * width / 100));
      heightPerc()->setText(widthPerc()->text());
    }
  }

  void onHeightPercChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    double height = heightPerc()->textDouble();

    heightPx()->setTextf("%d", (int)(sprite->height() * height / 100));

    if (lockRatio()->isSelected()) {
      widthPx()->setTextf("%d", (int)(sprite->width() * height / 100));
      widthPerc()->setText(heightPerc()->text());
    }
  }

  Context* m_ctx;
  int m_baseWidth;
  int m_baseHeight;
  Manager* m_filterManager = nullptr;
};

SpriteSizeCommand::SpriteSizeCommand()
  : Command("SpriteSize",
            "Sprite Size",
            CmdRecordableFlag)
{
  m_useUI = true;
  m_width = 0;
  m_height = 0;
  m_scaleX = 1.0;
  m_scaleY = 1.0;
  m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
}

Command* SpriteSizeCommand::clone() const
{
  return new SpriteSizeCommand(*this);
}

void SpriteSizeCommand::onLoadParams(const Params& params)
{
  std::string useUI = params.get("use-ui");
  m_useUI = (useUI.empty() || (useUI == "true"));

  std::string width = params.get("width");
  if (!width.empty()) {
    m_width = std::strtol(width.c_str(), NULL, 10);
  }
  else
    m_width = 0;

  std::string height = params.get("height");
  if (!height.empty()) {
    m_height = std::strtol(height.c_str(), NULL, 10);
  }
  else
    m_height = 0;

  std::string resize_method = params.get("resize-method");
  if (!resize_method.empty()) {
    if (resize_method == "bilinear")
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_BILINEAR;
    else if (resize_method == "rotsprite")
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_ROTSPRITE;
    else
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
  }
  else
    m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
}

bool SpriteSizeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpriteSizeCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  int new_width = (m_width ? m_width: int(sprite->width()*m_scaleX));
  int new_height = (m_height ? m_height: int(sprite->height()*m_scaleY));
  ResizeMethod resize_method = m_resizeMethod;

  if (m_useUI && context->isUIAvailable()) {
    SpriteSizeWindow window(context, new_width, new_height);
    window.remapWindow();
    window.centerWindow();

    load_window_pos(&window, "SpriteSize");
    window.setVisible(true);
    window.openWindowInForeground();
    save_window_pos(&window, "SpriteSize");

    if (window.closer() != window.ok())
      return;

    new_width  = window.evalWidthPx();
    new_height = window.evalHeightPx();
    resize_method = (ResizeMethod)window.method()->getSelectedItemIndex();

    set_config_int("SpriteSize", "Method", resize_method);
  }

  {
    SpriteSizeJob job(reader, new_width, new_height, resize_method);
    job.startJob();
    job.waitJob();
  }

  update_screen_for_document(reader.document());
}

Command* CommandFactory::createSpriteSizeCommand()
{
  return new SpriteSizeCommand;
}

} // namespace app
