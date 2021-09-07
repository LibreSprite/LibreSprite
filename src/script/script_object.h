// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/injection.h"
#include "function.h"

namespace script {
  class Engine;

  struct ObjectProperty {
    Function getter;
    Function setter;
  };

  class InternalScriptObject : public Injectable<InternalScriptObject> {
  public:
    virtual void makeGlobal(const std::string& name) = 0;
    virtual void makeLocal() = 0;

    virtual void addProperty(const std::string& name, const Function& get, const Function& set) {
      auto& prop = properties[name];
      prop.getter = get;
      prop.setter = set;
    }

    virtual Function& addFunction(const std::string& name, const Function& func) {
      return functions.emplace(name, func).first->second;
    }

    inject<script::Engine> m_engine;
    std::unordered_map<std::string, ObjectProperty> properties;
    std::unordered_map<std::string, Function> functions;
  };

  class ScriptObject : public Injectable<ScriptObject> {
  public:
    InternalScriptObject* getInternalScriptObject() {return m_internal;};

    virtual void* getWrapped(){return nullptr;}
    virtual void setWrapped(void*){}

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

    template<typename Type>
    Type* getWrapped(){ return static_cast<Type*>(getWrapped()); }

  protected:
    void makeGlobal(const std::string& name) {
      m_internal->makeGlobal(name);
    }

    void addProperty(const std::string& name, const Function& get = []{return Value{};}, const Function &set = [](const Value&){return Value{};}) {
      m_internal->addProperty(name, get, set);
    }

    Function& addFunction(const std::string& name, const Function& func) {
      return m_internal->addFunction(name, func);
    }

    template<typename Class, typename Ret, typename ... Args>
    Function& addMethod(const std::string& name, Class* instance, Ret(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        return (instance->*method)(std::forward<Args>(args)...);
      });
    }

    template<typename Class, typename ... Args>
    Function& addMethod(const std::string& name, Class* instance, void(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        (instance->*method)(std::forward<Args>(args)...);
        return Value{};
      });
    }

    template<typename Class, typename Ret, typename ... Args>
    Function& addMethod(const std::string& name, Ret(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        return (static_cast<Class*>(this)->*method)(std::forward<Args>(args)...);
      });
    }

    template<typename Class, typename ... Args>
    Function& addMethod(const std::string& name, void(Class::*method)(Args...)) {
      return addFunction(name, [=](Args ... args){
        (static_cast<Class*>(this)->*method)(std::forward<Args>(args)...);
        return Value{};
      });
    }

    inject<InternalScriptObject> m_internal;
  };

}
