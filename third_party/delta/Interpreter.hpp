#pragma once

#include <string>
#include <typeindex>

#include "JSON.hpp"

class Interpreter {
public:
    enum class EvalType { Module, Script };

    virtual ~Interpreter() {}
    virtual JSON::Value eval(const std::string& script, const std::string& path, EvalType type = EvalType::Module) = 0;
    virtual void tick() = 0;
    virtual void addGlobalValue(const std::string& name, JSON::Value& value) = 0;
    virtual void addAPIFunction(const std::string& name, const std::function<JSON::Value(JSON::Array&)>& func) = 0;
    virtual void addModuleSearchPath(const std::string& dir) = 0;
    virtual void reportError(const std::exception& e) = 0;

    using ClassCtor = std::function<std::shared_ptr<void>(JSON::Array&)>;

    class ClassDef {
    public:
        const ClassCtor& ctor;
        ClassDef(const ClassCtor& ctor) : ctor{ctor} {
            if (!ctor) {
                throw std::invalid_argument{"Constructor cannot be null"};
            }
        }

        ClassDef(const ClassDef&) = delete;
        ClassDef(ClassDef&&) = delete;
        virtual ~ClassDef() = default;
        virtual void addMethod(const std::string& name, const std::function<JSON::Value(void* self, JSON::Array&)>& func) = 0;
        virtual void addGetSet(const std::string& name, const std::function<JSON::Value(void* self)>& get, const std::function<void(void* self, JSON::Value&)>& set) = 0;
    };
    virtual ClassDef& addClass(const std::string& name, std::type_index base, std::type_index derived, const ClassCtor& ctor) = 0;
};
