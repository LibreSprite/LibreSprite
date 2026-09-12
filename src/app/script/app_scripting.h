// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/injection.h"
#include "script/value.h"

namespace script {
  class Engine;
};

class Extension;

namespace app {

  class AppScripting {
    static void initEngine();
    static void engineRaiseEvent(script::Value& event);
    static void addExtension(Extension& ext);
  public:
    static bool evalFile(const std::string& fileName);
    static void raiseEvent(const std::string& fileName, script::Value& event);
    static bool scanScript(const std::string& fullPath);
    static void clearEventHooks();

    static bool eval(const std::string& code, const std::string& path = "::");
    static void printLastResult();
  };

} // namespace app
