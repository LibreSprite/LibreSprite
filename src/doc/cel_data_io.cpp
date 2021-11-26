// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_data_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/cel_data.h"
#include "doc/subobjects_io.h"
#include "doc/user_data_io.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_celdata(std::ostream& os, const CelData* celdata)
{
  write32(os, celdata->id());
  write32(os, (int16_t)celdata->position().x);
  write32(os, (int16_t)celdata->position().y);
  write8(os, celdata->opacity());
  write32(os, celdata->image()->id());
  write_user_data(os, celdata->userData());
}

CelData* read_celdata(std::istream& is, SubObjectsIO* subObjects, bool setId)
{
  ObjectId id = read32(is);
  int x = read32(is);
  int y = read32(is);
  int opacity = read8(is);
  ObjectId imageId = read32(is);
  UserData userData = read_user_data(is);

  auto image = subObjects->getImageRef(imageId);
  if (!image)
    return nullptr;

  base::UniquePtr<CelData> celdata(new CelData(image));
  celdata->setPosition(x, y);
  celdata->setOpacity(opacity);
  celdata->setUserData(userData);
  if (setId)
    celdata->setId(id);
  return celdata.release();
}

}
