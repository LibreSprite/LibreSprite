// LibreSprite | Copyright (C) 2024       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include <cstring>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/ini_file.h"
#include "base/file_handle.h"
#include "doc/doc.h"

#include <stdio.h>
#include <stdlib.h>

#define QOI_IMPLEMENTATION
#include <qoi.h>

namespace app {

using namespace base;

class QoiFormat : public FileFormat {
  const char* onGetName() const override { return "qoi"; }
  const char* onGetExtensions() const override { return "qoi"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override;
};

FileFormat* CreateQoiFormat()
{
  return new QoiFormat;
}

bool QoiFormat::onLoad(FileOp* fop)
{
  qoi_desc desc;
  auto decoded = std::shared_ptr<void>(qoi_read(fop->filename().c_str(), &desc, 4), free);
  if (!decoded) {
    fop->setError("Could not load qoi");
    return false;
  }
  fop->sequenceSetHasAlpha(true);
  auto image = fop->sequenceImage(IMAGE_RGB, desc.width, desc.height);
  memcpy(image->getPixelAddress(0, 0), decoded.get(), desc.width * desc.height * 4);
  return true;
}

bool QoiFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  qoi_desc desc {
    .width = (unsigned int) image->width(),
    .height = (unsigned int) image->height(),
    .channels = 4,
    .colorspace = QOI_SRGB
  };
  return qoi_write(fop->filename().c_str(), image->getPixelAddress(0, 0), &desc) != 0;
}

} // namespace app
