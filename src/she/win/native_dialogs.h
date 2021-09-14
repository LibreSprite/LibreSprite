// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "she/native_dialogs.h"

namespace she {

  class NativeDialogsWin32 : public NativeDialogs {
  public:
    NativeDialogsWin32();
    FileDialog* createFileDialog() override;
  };

} // namespace she
