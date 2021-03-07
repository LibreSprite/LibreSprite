// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FILE_FILE_FORMAT_H_INCLUDED
#define APP_FILE_FILE_FORMAT_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <vector>

#define FILE_SUPPORT_LOAD               0x00000001
#define FILE_SUPPORT_SAVE               0x00000002
#define FILE_SUPPORT_RGB                0x00000004
#define FILE_SUPPORT_RGBA               0x00000008
#define FILE_SUPPORT_GRAY               0x00000010
#define FILE_SUPPORT_GRAYA              0x00000020
#define FILE_SUPPORT_INDEXED            0x00000040
#define FILE_SUPPORT_LAYERS             0x00000080
#define FILE_SUPPORT_FRAMES             0x00000100
#define FILE_SUPPORT_PALETTES           0x00000200
#define FILE_SUPPORT_SEQUENCES          0x00000400
#define FILE_SUPPORT_GET_FORMAT_OPTIONS 0x00000800
#define FILE_SUPPORT_FRAME_TAGS         0x00001000
#define FILE_SUPPORT_BIG_PALETTES       0x00002000 // Palettes w/more than 256 colors
#define FILE_SUPPORT_PALETTE_WITH_ALPHA 0x00004000

namespace app {

  class FormatOptions;
  class FileFormat;
  class FileOp;

  // A file format supported by ASE. It is the base class to extend if
  // you want to add support to load and/or save a new kind of
  // image/animation format.
  class FileFormat {
  public:
    FileFormat();
    virtual ~FileFormat();

    const char* name() const;       // File format name
    const char* extensions() const; // Extensions (e.g. "jpeg,jpg")
    bool load(FileOp* fop);
    bool save(FileOp* fop);

    // Does post-load operation which require user intervention.
    // Returns false cancelled the operation.
    bool postLoad(FileOp* fop);

    // Destroys the custom data stored in "fop->format_data" field.
    void destroyData(FileOp* fop);

    // Returns extra options for this format. It can return != NULL
    // only if flags() returns FILE_SUPPORT_GET_FORMAT_OPTIONS.
    base::SharedPtr<FormatOptions> getFormatOptions(FileOp* fop) {
      return onGetFormatOptions(fop);
    }

    // Returns true if this file format supports the given flag.
    bool support(int f) const {
      return ((onGetFlags() & f) == f);
    }

  protected:
    virtual const char* onGetName() const = 0;
    virtual const char* onGetExtensions() const = 0;
    virtual int onGetFlags() const = 0;

    virtual bool onLoad(FileOp* fop) = 0;
    virtual bool onPostLoad(FileOp* fop) { return true; }
    virtual bool onSave(FileOp* fop) = 0;
    virtual void onDestroyData(FileOp* fop) { }

    virtual base::SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) {
      return base::SharedPtr<FormatOptions>(0);
    }

  };

} // namespace app

#endif
