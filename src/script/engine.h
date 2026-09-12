// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/with_handle.h"
#include <unordered_map>
#include "base/injection.h"
#include "value.h"

namespace script {
  class Engine : public Injectable<Engine> {
  protected:
    bool m_printLastResult = false;
  public:
    virtual void printLastResult() { m_printLastResult = true; }
    bool getPrintLastResult() {return m_printLastResult;}

    virtual bool eval(const std::string& code) = 0;
    virtual bool raiseEvent(script::Value& event) = 0;

  };
}
