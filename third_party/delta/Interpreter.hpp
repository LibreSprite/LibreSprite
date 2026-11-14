#pragma once

#include <string>
#include <typeindex>
#include <vector>

#include "JSON.hpp"

class Interpreter {
public:
    enum class EvalType { Module, Script };

    // Host "script context" token stack: identifies whose JS code is
    // running. The host pushes a token at every JS entry point.
    static void pushToken(const void* token) {
        tokenStack().push_back(token);
    }
    static void popToken() {
        auto& stack = tokenStack();
        if (!stack.empty())
            stack.pop_back();
    }
    static const void* currentToken() {
        auto& stack = tokenStack();
        return stack.empty() ? nullptr : stack.back();
    }
    // True when the token is active somewhere on the stack (the owning
    // JS is running, possibly nested under other envs' tokens).
    static bool hasToken(const void* token) {
        for (auto* t : tokenStack())
            if (t == token)
                return true;
        return false;
    }

    // RAII: pushes a token on construction, pops it on destruction.
    class TokenGuard {
    public:
        explicit TokenGuard(const void* token) { pushToken(token); }
        ~TokenGuard() { popToken(); }
        TokenGuard(const TokenGuard&) = delete;
        TokenGuard& operator=(const TokenGuard&) = delete;
    };

    // The script context token this interpreter's JS runs under (set once
    // at bootstrap): the interpreter pushes it around its own JS entry
    // points (eval, tick), so the jobs and microtasks they drain run
    // under the owning script's token. Main thread only.
    virtual void setToken(const void* token) {}

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

private:
    static std::vector<const void*>& tokenStack() {
        static std::vector<const void*> stack;
        return stack;
    }
};
