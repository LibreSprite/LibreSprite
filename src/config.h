// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2018-2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef __ASE_CONFIG_H
#error You cannot use config.h two times
#endif

#define __ASE_CONFIG_H

// In MSVC
#ifdef _MSC_VER
  // Avoid warnings about insecure standard C++ functions
  #ifndef _CRT_SECURE_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
  #endif

  // Disable warning C4355 in MSVC: 'this' used in base member initializer list
  #pragma warning(disable:4355)
#endif

// General information
#define PACKAGE "LibreSprite"
#define VERSION "1.0-dev"

#define WEBSITE                 "https://github.com/LibreSprite/LibreSprite/"
#define WEBSITE_DOWNLOAD        WEBSITE "releases/"
#define WEBSITE_CONTRIBUTORS    WEBSITE "graphs/contributors/"
#define COPYRIGHT               "Copyright (C) 2001-2016 David Capello, 2016-2021 LibreSprite contributors"

#include "base/base.h"
#include "base/debug.h"
#include "base/log.h"