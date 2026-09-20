// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/file/svg_dialect.h"
#include "app/file/svg_options.h"
#include "app/ini_file.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "gfx/color.h"
#include "she/surface.h"
#include "she/system.h"

#include "svg_options.xml.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <map>
#include <memory>
#include <string>

namespace app {

using namespace base;

namespace {

static void appendInt(std::string& out, int value)
{
  char buf[16];
  auto res = std::to_chars(buf, buf + sizeof(buf), value);
  out.append(buf, res.ptr);
}

// Converts any pixel to RGBA. Indexed transparency is the mask entry
// (only without a visible background layer). Alpha 0 stays uncovered.
static doc::color_t pixelRgba(const Image* image, int x, int y,
                              FileOp* fop, int maskEntry)
{
  color_t c = get_pixel(image, x, y);
  switch (image->pixelFormat()) {
    case IMAGE_RGB:
      return c;

    case IMAGE_GRAYSCALE: {
      int v = graya_getv(c);
      return rgba(v, v, v, graya_geta(c));
    }

    case IMAGE_INDEXED: {
      if ((int)c == maskEntry)
        return rgba(0, 0, 0, 0);
      // FileOp holds the frame's palette for each onSave call.
      int r = 0, g = 0, b = 0, a = 255;
      fop->sequenceGetColor(c, &r, &g, &b);
      fop->sequenceGetAlpha(c, &a);
      return rgba(r, g, b, a);
    }

    default:
      return rgba(0, 0, 0, 0);
  }
}

// Tries the system image loader first; falls back to SvgDialect.
static bool loadWithSystemDecoder(FileOp* fop)
{
  std::shared_ptr<she::Surface> surface;
  try {
    surface.reset(she::instance()->loadRgbaSurface(fop->filename().c_str()));
  }
  catch (...) {
    return false;
  }
  if (!surface || surface->width() <= 0 || surface->height() <= 0)
    return false;

  Image* image = fop->sequenceImage(IMAGE_RGB, surface->width(), surface->height());
  if (!image)
    return false;

  bool hasAlpha = false;
  for (int y = 0; y < image->height(); ++y) {
    for (int x = 0; x < image->width(); ++x) {
      gfx::Color c = surface->getPixel(x, y);
      if (gfx::geta(c) < 255)
        hasAlpha = true;
      put_pixel(image, x, y,
                doc::rgba(gfx::getr(c), gfx::getg(c), gfx::getb(c), gfx::geta(c)));
    }
    fop->setProgress(double(y + 1) / double(image->height()));
    if (fop->isStop())
      return false;
  }
  fop->sequenceSetHasAlpha(hasAlpha);
  return true;
}

} // anonymous namespace

class SvgFormat : public FileFormat {
public:
  const char* onGetName() const override { return "svg"; }
  const char* onGetExtensions() const override { return "svg"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override;

  base::SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) override;
};

static FileFormat::Regular<SvgFormat> ff{"svg"};

// Keeps svg_format linked into test binaries (see tests).
extern "C" void svg_format_anchor() { }

bool SvgFormat::onLoad(FileOp* fop)
{
  return loadWithSystemDecoder(fop) || SvgDialect::load(fop);
}

bool SvgFormat::onSave(FileOp* fop)
{
  // Document may hold another format's options from a previous save.
  int scale = 1;
  if (SvgOptions* svg_options = dynamic_cast<SvgOptions*>(fop->sequenceGetFormatOptions().get()))
    scale = std::clamp(svg_options->scale(), kSvgMinScale, kSvgMaxScale);

  const Image* image = fop->sequenceImage();
  if (!image) {
    fop->setError("No image to save\n");
    return false;
  }
  const int width = image->width();
  const int height = image->height();

  FileHandle handle;
  try {
    handle = open_file_with_exception(fop->filename(), "wb");
  }
  catch (const std::exception& e) {
    fop->setError("Error opening file: %s\n", e.what());
    return false;
  }

  // Mask entry applies only without a visible background layer.
  int maskEntry = -1;
  if (fop->document() && fop->document()->sprite()) {
    if (fop->document()->sprite()->backgroundLayer() == nullptr ||
        !fop->document()->sprite()->backgroundLayer()->isVisible())
      maskEntry = fop->document()->sprite()->transparentColor();
  }

  // One <path> per color; each run is a 1px-tall rectangle
  // ("M x y h w v1 h -w z"). Rectangles never overlap, so holes
  // need no boundary tracing.
  std::map<doc::color_t, std::string> pathData;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ) {
      doc::color_t color = pixelRgba(image, x, y, fop, maskEntry);
      if (rgba_geta(color) == 0) {
        ++x;
        continue;
      }

      int end = x + 1;
      while (end < width && pixelRgba(image, end, y, fop, maskEntry) == color)
        ++end;

      std::string& d = pathData[color];
      d += "M";
      appendInt(d, x);
      d += " ";
      appendInt(d, y);
      d += "h";
      appendInt(d, end - x);
      d += "v1h-";
      appendInt(d, end - x);
      d += "z ";
      x = end;
    }
    fop->setProgress(0.9 * double(y + 1) / double(height));
    if (fop->isStop()) {
      fop->setError("Save cancelled\n");
      return false;
    }
  }

  // viewBox stays in sprite pixels; width/height carry the scale.
  // Buffered and written with a single checked write.
  char header[256];
  std::snprintf(header, sizeof(header),
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" "
    "width=\"%d\" height=\"%d\" shape-rendering=\"crispEdges\">\n",
    width, height, width * scale, height * scale);

  std::string out(header);
  size_t emitted = 0;
  for (const auto& [color, d] : pathData) {
    if (fop->isStop()) {
      fop->setError("Save cancelled\n");
      return false;
    }
    ++emitted;

    char fill[32];
    std::snprintf(fill, sizeof(fill), "#%02x%02x%02x",
                  rgba_getr(color), rgba_getg(color), rgba_getb(color));
    out += "  <path fill=\"";
    out += fill;
    if (rgba_geta(color) < 255) {
      // Six digits keep the alpha byte exact on round trip.
      char opacity[16];
      std::snprintf(opacity, sizeof(opacity), "%.6g", rgba_geta(color) / 255.0);
      out += "\" fill-opacity=\"";
      out += opacity;
    }
    out += "\" fill-rule=\"nonzero\" d=\"";
    out += d;
    out += "\"/>\n";

    fop->setProgress(0.9 + 0.1 * double(emitted) / double(pathData.size()));
  }
  out += "</svg>\n";

  if (out.size() != fwrite(out.data(), 1, out.size(), handle.get()) ||
      fflush(handle.get()) != 0 ||
      ferror(handle.get())) {
    fop->setError("Error writing file \"%s\"\n", fop->filename().c_str());
    return false;
  }
  return true;
}

base::SharedPtr<FormatOptions> SvgFormat::onGetFormatOptions(FileOp* fop)
{
  base::SharedPtr<SvgOptions> svg_options;
  if (fop->document() && fop->document()->getFormatOptions()) {
    // Same as onSave: options may belong to another format.
    SvgOptions* existing =
      dynamic_cast<SvgOptions*>(fop->document()->getFormatOptions().get());
    if (existing)
      svg_options.reset(new SvgOptions(*existing));
  }

  if (!svg_options)
    svg_options.reset(new SvgOptions);

  // Persisted default; used as-is without UI (CLI, web).
  svg_options->setScale(get_config_int("SVG", "Scale", svg_options->scale()));

  // Non-interactive mode
  if (!fop->context() ||
      !fop->context()->isUIAvailable())
    return svg_options;

  try {
    int spriteW = 0, spriteH = 0;
    if (fop->document() && fop->document()->sprite()) {
      spriteW = fop->document()->sprite()->width();
      spriteH = fop->document()->sprite()->height();
    }

    // Ask the user for the SVG options.
    app::gen::SvgOptions win;

    auto updateInfo = [&]() {
      int s = win.scale()->getValue();
      char buf[128];
      if (spriteW > 0 && spriteH > 0) {
        snprintf(buf, sizeof(buf), "%dx%d px -> %dx%d px vector",
                 spriteW, spriteH, spriteW * s, spriteH * s);
      }
      else {
        snprintf(buf, sizeof(buf), "Scale %dx (sharp at all sizes)", s);
      }
      win.info()->setText(buf);
    };

    win.scale()->setValue(std::clamp(svg_options->scale(),
                                     win.scale()->getMinValue(),
                                     win.scale()->getMaxValue()));
    win.scale()->Change.connect(updateInfo);
    updateInfo();

    win.openWindowInForeground();

    if (win.closer() == win.ok()) {
      svg_options->setScale(win.scale()->getValue());
      set_config_int("SVG", "Scale", svg_options->scale());
    }
    else {
      svg_options.reset(NULL);
    }

    return svg_options;
  }
  catch (std::exception& e) {
    Console::showException(e);
    return base::SharedPtr<FormatOptions>(0);
  }
}

} // namespace app
