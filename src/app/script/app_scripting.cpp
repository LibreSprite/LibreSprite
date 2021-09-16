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
#include "base/path.h"
#include "base/string.h"
#include "script/engine.h"
#include "script/engine_delegate.h"
#include <fstream>

namespace {

inject<script::Engine> engine{nullptr};
std::string previousFileName;

}

namespace app {
  std::string AppScripting::m_fileName;

  void AppScripting::initEngine() {
    // if there is no engine OR
    // the engine we have doesn't match the default in the registry,
    // inject a new one
    if (!engine || !script::Engine::getRegistry()[""].match(engine.get()))
        engine = inject<script::Engine>();
  }

  void AppScripting::raiseEvent(const std::string& fileName, const std::string &event) {
    if (fileName == previousFileName || evalFile(fileName))
      engine->raiseEvent(event);
  }

  bool AppScripting::eval(const std::string& code) {
    initEngine();
    if (engine) {
      return engine->eval(code);
    }
    inject<script::EngineDelegate>{}->onConsolePrint("No compatible scripting engine.");
    return false;
  }

  bool AppScripting::evalFile(const std::string& fileName) {
    m_fileName = fileName;
    std::cout << "Reading file " << fileName << std::endl;
    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cout << "Could not open " << fileName << std::endl;
      return false;
    }

    auto extension = base::string_to_lower(base::get_file_extension(fileName));
    script::Engine::setDefault(extension, {extension});

    engine = nullptr;

    AppScripting instance;
    if (!instance.eval({std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()}))
      return false;

    engine->raiseEvent("init");

    previousFileName = fileName;
    return true;
  }

  void AppScripting::printLastResult() {
    if(engine)
      engine->printLastResult();
  }

}
