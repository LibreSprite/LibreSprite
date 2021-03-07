// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_format.h"
#include "app/file/format_options.h"

#include <algorithm>

namespace app {

FileFormat::FileFormat()
{
}

FileFormat::~FileFormat()
{
}

const char* FileFormat::name() const
{
  return onGetName();
}

const char* FileFormat::extensions() const
{
  return onGetExtensions();
}

bool FileFormat::load(FileOp* fop)
{
  ASSERT(support(FILE_SUPPORT_LOAD));
  return onLoad(fop);
}

bool FileFormat::save(FileOp* fop)
{
  ASSERT(support(FILE_SUPPORT_SAVE));
  return onSave(fop);
}

bool FileFormat::postLoad(FileOp* fop)
{
  return onPostLoad(fop);
}

void FileFormat::destroyData(FileOp* fop)
{
  onDestroyData(fop);
}

} // namespace app
