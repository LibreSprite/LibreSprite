// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/app.h"
#include "app/console.h"
#include "script/engine.h"
#include <iostream>

class ConsoleScriptObject : public script::ScriptObject {
public:
  ConsoleScriptObject() {
    addMethod("log", this, &ConsoleScriptObject::log);
    addMethod("assert", this, &ConsoleScriptObject::_assert);
    makeGlobal("console");
  }

  void _assert(bool condition, const std::string& msg){
    if (!condition)
      log(msg);
  }

  void log(const std::string& str) {
    std::cout << str << std::endl;
    if (app::App::instance()->isGui()) {
      app::Console().printf("%s\n", str.c_str());
    }
  }
};

static script::ScriptObject::Regular<ConsoleScriptObject> reg("ConsoleScriptObject", {"global"});
