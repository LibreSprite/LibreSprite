// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/app.h"
#include "app/console.h"
#include "script/engine.h"
#include "script/engine_delegate.h"
#include <iostream>
#include <sstream>

class ConsoleScriptObject : public script::ScriptObject {
public:
  inject<script::EngineDelegate> delegate;

  ConsoleScriptObject() {
    addMethod("log", this, &ConsoleScriptObject::log);
    addMethod("verbose", this, &ConsoleScriptObject::verbose);
    addMethod("assert", this, &ConsoleScriptObject::conditionalLog);
    makeGlobal("console");
  }

  void conditionalLog(bool condition, const std::string& msg){
    if (!condition)
      log();
  }

  void verbose() {
    bool first = true;
    for (auto& arg : script::Function::varArgs()) {
      if (!first) {
          std::cout << " ";
      }
      first = false;
      std::cout << arg.str();
    }
    std::cout << std::endl;
  }

  void log() {
    std::stringstream stream;
    bool first = true;
    for (auto& arg : script::Function::varArgs()) {
      if (!first) {
        stream << " ";
      }
      first = false;
      stream << arg.str();
    }
    delegate->onConsolePrint(stream.str().c_str());
  }
};

static script::ScriptObject::Regular<ConsoleScriptObject> reg("ConsoleScriptObject", {"global"});
