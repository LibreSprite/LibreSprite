// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/script/app_scripting.h"
#include "base/file_handle.h"
#include "script/engine.h"
#include "script/engine_delegate.h"

namespace app {

  void AppScripting::eval(const std::string& code) {
    if (m_engine) {
      m_engine->eval(code);
    } else {
      inject<script::EngineDelegate>{}->onConsolePrint("No compatible scripting engine.");
    }
  }

  void AppScripting::evalFile(const std::string& fileName) {
    base::FileHandle fhandle(base::open_file(fileName, "rb"));
    if (!fhandle)
      return;

    FILE* f = fhandle.get();
    if (!f)
      return;

    if (fseek(f, 0, SEEK_END) < 0)
      return;

    int sz = ftell(f);
    if (sz < 0)
      return;

    if (fseek(f, 0, SEEK_SET) < 0)
      return;

    std::string code;
    code.resize(sz + 1, 0);
    if (fread(&code[0], 1, sz, f) != static_cast<size_t>(sz))
      return;
    eval(code);
  }

  void AppScripting::printLastResult() {
    if(m_engine)
      m_engine->printLastResult();
  }

}
