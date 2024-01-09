// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/injection.h"

namespace script {
    class Engine;
};

namespace app {

  class AppScripting {
    void initEngine();
    static std::string m_fileName;

  public:
    static const std::string& getFileName() {return m_fileName;}
    static bool evalFile(const std::string& fileName);
    static void raiseEvent(const std::string& fileName, const std::vector<std::string>& event);
    static bool scanScript(const std::string& fullPath);
    static void clearEventHooks();

    bool eval(const std::string& code);
    void printLastResult();
  };

} // namespace app
