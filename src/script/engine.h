// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "script/value.h"
#pragma once

#include <string>
#include "script_object.h"

namespace script {
  class Engine : public Injectable<Engine> {
  public:
    void initGlobals() {
      m_scriptObjects = ScriptObject::getAllWithFlag("global");
    }

    virtual void printLastResult() = 0;
    virtual bool eval(const std::string& code) = 0;

  private:
    Provides m_provides{this};
    std::vector<inject<ScriptObject>> m_scriptObjects;
  };
}
