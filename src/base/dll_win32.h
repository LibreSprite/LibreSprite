// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2018-2022  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/string.h"
#include <windows.h>

namespace base {

dll load_dll(const std::string& filename)
{
  return LoadLibraryW(base::from_utf8(filename).c_str());
}

void unload_dll(dll lib)
{
  FreeLibrary((HMODULE)lib);
}

dll_proc get_dll_proc_base(dll lib, const char* procName)
{
  return reinterpret_cast<dll_proc>(
    GetProcAddress((HMODULE)lib, procName));
}


} // namespace base
