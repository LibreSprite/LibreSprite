// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/with_handle.h"
#include "script/script_object.h"
#include "script/value.h"
#include <unordered_map>

namespace script {
  class Engine : public Injectable<Engine> {
  protected:
    bool m_printLastResult = false;

    void execAfterEval(bool success) {
      for (auto& listener : m_afterEvalListeners) {
        listener(success);
      }
      m_afterEvalListeners.clear();
      for (auto it = m_ObjToScriptObj.begin(); it != m_ObjToScriptObj.end();) {
        auto& entry = it->second;
        if (!entry->held && entry->script->disposable()) {
          it = m_ObjToScriptObj.erase(it);
        } else {
          ++it;
        }
      }
      for (auto it = m_scriptObjects.begin(); it != m_scriptObjects.end();) {
        auto& entry = *it;
        if (!entry->held && entry->script->disposable()) {
          it = m_scriptObjects.erase(it);
        } else {
          ++it;
        }
      }
    }

  public:

    void initGlobals() {
      if (m_scriptObjects.empty()) {
        for (auto& obj : ScriptObject::getAllWithFlag("global"))
          m_scriptObjects.push_back(std::make_shared<HeldObject>(std::move(obj)));
      }
    }

    virtual void printLastResult() { m_printLastResult = true; }
    bool getPrintLastResult() {return m_printLastResult;}

    virtual bool eval(const std::string& code) = 0;
    virtual bool raiseEvent(const std::vector<script::Value>& event) = 0;

    void afterEval(std::function<void(bool)>&& callback) {
      m_afterEvalListeners.emplace_back(std::move(callback));
    }

    template<typename Type>
    ScriptObject* getScriptObject(Type* obj, bool own = false) {
      if (!obj) {
        return nullptr;
      }
      if (auto it = m_ObjToScriptObj.find(obj); it != m_ObjToScriptObj.end() && it->second->handle) {
        return it->second->script;
      }
      inject<ScriptObject> sobj{typeid(obj).name()};
      if (!sobj) {
        return nullptr;
      }
      sobj->setWrapped(obj->handle(), own);
      return registerScriptObject(std::move(sobj));
    }

    ScriptObject* create(const std::string& name) {
      inject<ScriptObject> sobj{name};
      if (!sobj) {
        std::cout << "Unknown ScriptObject type: " << name << std::endl;
        return nullptr;
      }
      if (!sobj->create<void>())
        return nullptr;
      return registerScriptObject(std::move(sobj));
    }

    template <typename Type>
    ScriptObject* create() {
      return create(typeid(Type*).name());
    }

    ScriptObject* registerScriptObject(inject<ScriptObject>&& sobj) {
      ScriptObject* ret = &*sobj;
      auto raw = sobj->m_handle.get<void>();
      sobj->getInternalScriptObject()->onRelease = [=, this]{
        auto it = m_ObjToScriptObj.find(raw);
        if (it == m_ObjToScriptObj.end())
          return;
        it->second->held = false;
      };
      auto hold = std::make_shared<HeldObject>(std::move(sobj), sobj->m_handle);
      m_ObjToScriptObj.emplace(std::make_pair(raw, hold));
      m_scriptObjects.push_back(hold);
      return ret;
    }

  private:
    Provides m_provides{this};

    struct HeldObject {
      Handle handle;
      inject<ScriptObject> script;
      bool held = true;
      HeldObject(inject<ScriptObject>&& script, const Handle& handle = {}) : handle{handle}, script{std::move(script)} {}
    };

    std::unordered_map<void*, std::shared_ptr<HeldObject>> m_ObjToScriptObj;
    std::vector<std::shared_ptr<HeldObject>> m_scriptObjects;
    std::vector<std::function<void(bool)>> m_afterEvalListeners;
  };
}
