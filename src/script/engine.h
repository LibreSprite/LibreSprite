// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "script/script_object.h"
#include "script/value.h"

namespace script {
  class Engine : public Injectable<Engine> {
  protected:
    void execAfterEval(bool success) {
      for (auto& listener : m_afterEvalListeners) {
        listener(success);
      }
      m_afterEvalListeners.clear();
    }

  public:

    void initGlobals() {
      if (m_scriptObjects.empty())
        m_scriptObjects = ScriptObject::getAllWithFlag("global");
    }

    virtual void printLastResult() = 0;
    virtual bool eval(const std::string& code) = 0;
    virtual bool raiseEvent(const std::string& event) = 0;

    void afterEval(std::function<void(bool)>&& callback) {
      m_afterEvalListeners.emplace_back(std::move(callback));
    }

  private:
    Provides m_provides{this};
    std::vector<inject<ScriptObject>> m_scriptObjects;
    std::vector<std::function<void(bool)>> m_afterEvalListeners;
  };
}
