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
  public:
    void eval(const std::string& code);
    void evalFile(const std::string& fileName);
    void printLastResult();

  private:
    inject<script::Engine> m_engine;
  };

} // namespace app
