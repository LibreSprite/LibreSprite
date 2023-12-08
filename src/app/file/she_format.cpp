// LibreSprite | Copyright (C) 2023       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//

#include "doc/color.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "doc/pixel_format.h"
#include "render/render.h"
#include "she/system.h"
#include "she/surface.h"
#include "she/surface_format.h"
#include <memory>

namespace app {

using namespace base;

class SheFormat : public FileFormat {
  const char* onGetName() const override { return "she"; }
  const char* onGetExtensions() const override { return ""; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SEQUENCES;
  }

  int loadPriority() override {return 2;}

  bool onSave(FileOp* fop) override {return false;}

  bool onLoad(FileOp* fop) override;
};

FileFormat* CreateSheFormat()
{
  return new SheFormat;
}

bool SheFormat::onLoad(FileOp* fop)
{
  try {
    auto surface = std::shared_ptr<she::Surface>(she::instance()->loadRgbaSurface(fop->filename().c_str()));
    auto width = surface->width();
    auto height = surface->height();
    Image* image = fop->sequenceImage(IMAGE_RGB, width, height);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto c = surface->getPixel(x, y);
        auto r = gfx::getr(c);
        auto g = gfx::getg(c);
        auto b = gfx::getb(c);
        auto a = gfx::geta(c);
        put_pixel(image, x, y, doc::rgba(r,g,b,a));
      }
    }
    return true;
  } catch (...) {}

  return false;
}

} // namespace app
