// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/injection.h"
#include "base/with_handle.h"
#include "function.h"
#include <exception>

namespace script {
  class Engine;

  class ObjectDestroyedException : public std::exception {};


  struct ObjectProperty {
    Function getter;
    Function setter;
    std::string docStr;

    ObjectProperty& doc(const std::string& str) {
      docStr = str;
      return *this;
    }
  };

  struct DocumentedFunction : public Function {
    struct DocArg {
      std::string name;
      std::string docStr;
    };
    std::string docStr;
    std::vector<DocArg> docArgs;
    std::string docReturnsStr = "Nothing";

    DocumentedFunction(const Function& other) : Function(other) {}

    DocumentedFunction(Function&& other) : Function(std::move(other)) {}

    DocumentedFunction& doc(const std::string& str) {
      docStr = str;
      return *this;
    }

    DocumentedFunction& docArg(const std::string& arg, const std::string& doc) {
      docArgs.emplace_back(DocArg{arg, doc});
      return *this;
    }

    DocumentedFunction& docReturns(const std::string& doc) {
      docReturnsStr = doc;
      return *this;
    }
  };

  class InternalScriptObject : public Injectable<InternalScriptObject> {
  public:
    virtual void makeGlobal(const std::string& name) = 0;

    virtual ObjectProperty& addProperty(const std::string& name, const Function& get, const Function& set) {
      auto& prop = properties[name];
      prop.getter = get;
      prop.setter = set;
      return prop;
    }

    virtual DocumentedFunction& addFunction(const std::string& name, const Function& func) {
      return functions.emplace(name, func).first->second;
    }

    inject<script::Engine> m_engine;
    std::unordered_map<std::string, ObjectProperty> properties;
    std::unordered_map<std::string, DocumentedFunction> functions;
    std::function<void()> onRelease;
  };

  class ScriptObject : public Injectable<ScriptObject> {
    friend class Engine;
    Handle m_handle;
    bool m_own;

  public:
    #if _DEBUG
    static inline std::size_t count{};

    ScriptObject() {
      count++;
      std::cout << "+Object count: " << count << std::endl;
    }

    ~ScriptObject() {
      count--;
      std::cout << "-Object count: " << count << std::endl;
      if (m_own) {
        m_handle.dispose();
      }
    }
    #endif

    template <typename Base, typename Cast = Base>
    Cast* handle() {return m_handle.get<Base, Cast>();}

    template <typename Base, typename Cast = Base>
    Cast* create() {
      setWrapped(build(), true);
      return handle<Base, Cast>();
    }

    virtual Handle build() {return {};}

    virtual bool disposable() {return true;}

    InternalScriptObject* getInternalScriptObject() {return m_internal;};
    script::Engine* getEngine() {return &*m_internal->m_engine;}

    virtual void setWrapped(const Handle& handle, bool own) {
      if (m_own) {
        m_handle.dispose();
      }
      m_handle = handle;
      m_own = own;
    }

    template <typename Type = Value>
    Type get(const std::string& name) {
      auto it = m_internal->properties.find(name);
      if (it == m_internal->properties.end())
        return Value{};
      it->second.getter();
      return it->second.getter.result;
    }

    void set(const std::string& name, const Value& value) {
      auto it = m_internal->properties.find(name);
      if (it != m_internal->properties.end()) {
        auto& setter = it->second.setter;
        setter.arguments.emplace_back(value);
        setter();
      }
    }

    template <typename RetType = Value, typename ... Args>
    RetType call(const std::string& name, Args ... args) {
      auto it = m_internal->functions.find(name);
      if (it == m_internal->functions.end())
        return Value{};
      auto& func = it->second;
      func.arguments = {args...};
      func();
      return func.result;
    }

  protected:
    void makeGlobal(const std::string& name) {
      m_internal->makeGlobal(name);
    }

    ObjectProperty& addProperty(const std::string& name, const Function& get = []{return Value{};}, const Function &set = [](const Value&){return Value{};}) {
      return m_internal->addProperty(name, get, set);
    }

    DocumentedFunction& addFunction(const std::string& name, const Function& func) {
      return m_internal->addFunction(name, func);
    }

    template<typename Class, typename Ret, typename ... Args>
    DocumentedFunction& addMethod(const std::string& name, Class* instance, Ret(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        return (instance->*method)(std::forward<Args>(args)...);
      });
    }

    template<typename Class, typename ... Args>
    DocumentedFunction& addMethod(const std::string& name, Class* instance, void(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        (instance->*method)(std::forward<Args>(args)...);
        return Value{};
      });
    }

    template<typename Class, typename Ret, typename ... Args>
    DocumentedFunction& addMethod(const std::string& name, Ret(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        return (static_cast<Class*>(this)->*method)(std::forward<Args>(args)...);
      });
    }

    template<typename Class, typename ... Args>
    DocumentedFunction& addMethod(const std::string& name, void(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        (static_cast<Class*>(this)->*method)(std::forward<Args>(args)...);
        return Value{};
      });
    }

    inject<InternalScriptObject> m_internal;
  };

}
