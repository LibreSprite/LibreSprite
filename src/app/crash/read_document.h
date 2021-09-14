// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/crash/raw_images_as.h"
#include "doc/frame.h"
#include "doc/pixel_format.h"

#include <string>

namespace app {
class Document;
namespace crash {

  struct DocumentInfo {
    doc::PixelFormat format;
    int width;
    int height;
    doc::frame_t frames;
    std::string filename;

    DocumentInfo() :
      format(doc::IMAGE_RGB),
      width(0),
      height(0),
      frames(0) {
    }
  };

  bool read_document_info(const std::string& dir, DocumentInfo& info);
  app::Document* read_document(const std::string& dir);
  app::Document* read_document_with_raw_images(const std::string& dir,
                                               RawImagesAs as);

} // namespace crash
} // namespace app
