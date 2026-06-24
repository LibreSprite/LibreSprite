// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"

// HAVE_DLFCN_H comes from the host-generated config.h, which may be produced on
// a platform without dlfcn.h (e.g. the Windows codegen host) even when the
// actual target (Android/Linux/macOS) always provides it. Fall back to the
// known-present header on those targets.
#if defined(HAVE_DLFCN_H) || defined(__ANDROID__) || defined(__unix__) || defined(__linux__) || defined(__APPLE__)
  #include <dlfcn.h>
#else
  #error dlfcn.h is needed or include a file that defines dlopen/dlclose
#endif

namespace base {

dll load_dll(const std::string& filename)
{
  return dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
}

void unload_dll(dll lib)
{
  dlclose(lib);
}

dll_proc get_dll_proc_base(dll lib, const char* procName)
{
  return dlsym(lib, procName);
}

} // namespace base
