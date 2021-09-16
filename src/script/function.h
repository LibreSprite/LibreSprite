// LibreSprite Scripting Library
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

/*

The script::Function exists to make it easy for scripting engines to call native code.
It does this by providing a way to call any function using an array of arguments:

// simple function that we want to call from a script
int testFunc(int x, int y){ return x + y; }

script::Function func(testFunc); // create a wrapper
func.arguments.push_back(5); // add argument X
func.arguments.push_back(7); // add argument Y
func(); // call testFunc with the previously given arguments
int result = func.result; // result = 12

The arguments vector is automatically cleared after a call.
If an insufficient amount of arguments are provided, they will be default-constructed.

*/

#pragma once

#include <type_traits>
#include <vector>
#include <functional>
#include "value.h"

namespace script {

  class Function {

    // For generic types, directly use the result of the signature of its 'operator()'
    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {};

    template <typename ReturnType, typename ... Args>
    struct function_traits<ReturnType(&)(Args...)> {
      enum { arity = sizeof...(Args) }; // arity is the number of arguments.
      typedef ReturnType result_type;

      template <typename Func, bool, typename Tuple, unsigned int ...I>
      struct helper {
        static Value call(Func&& func, Value* args){
          return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, args);
        }
      };

      template <typename Func, unsigned int ...I>
      struct helper<Func, true, std::tuple<Args...>, I...> {
        static Value call(Func&& func, Value* args) {
          return func(static_cast<Args>(args[I])...);
        }
      };

      template <typename Func>
      static Value call(Func&& func, Value * args) {
        return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, args);
      }
    };

    // we specialize for pointers to member function
    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
      enum { arity = sizeof...(Args) }; // arity is the number of arguments.

      typedef ReturnType result_type;

      template <typename Func, bool, typename Tuple, unsigned int ...I>
      struct helper {
        static Value call(Func&& func, Value* args){
          return helper<Func, sizeof...(I) + 1 == std::tuple_size<Tuple>::value, Tuple, I..., sizeof...(I)>::call(func, args);
        }
      };

      template <typename Func, unsigned int ...I>
      struct helper<Func, true, std::tuple<Args...>, I...> {
        static Value call(Func&& func, Value* args) {
          return func(static_cast<Args>(args[I])...);
        }
      };

      template <typename Func>
      static Value call(Func&& func, Value * args) {
        return helper<Func, sizeof...(Args) == 0, std::tuple<Args...>>::call(func, args);
      }
    };

    std::function<void(Value&, std::vector<Value>&)> call;
    std::size_t argCount;

    static inline std::vector<Value>** getVarArgsPtr() {
      static std::vector<Value>* ptr = nullptr;
      return &ptr;
    }

  public:
    std::vector<Value> defaults;
    std::vector<Value> arguments;
    Value result;

    static std::vector<Value>& varArgs() {
      return **getVarArgsPtr();
    }

    Function() : call([](Value& ret, std::vector<Value>&){}), argCount(0) {}

    Function(Function&& other) {
      argCount = other.argCount;
      call = other.call;
    }

    Function(const Function& other) = default;

    Function(const Function&& other) : call(other.call), argCount(other.argCount) {}

    Function& operator = (Function&& other) = default;

    Function& operator = (const Function& other) = default;

    template<typename NativeFunction>
    Function(NativeFunction&& func) : argCount(function_traits<NativeFunction>::arity) {
      call = [func](Value& result, std::vector<Value>& arguments) {
        *getVarArgsPtr() = &arguments;
        result = function_traits<NativeFunction>::call(func, arguments.data());
      };
    }

    template<typename ... Arg>
    void setDefault(Arg ... arg) {
      defaults = {std::forward<Arg>(arg)...};
    }

    void operator () () {
      for (int i = 0, max = std::min(defaults.size(), argCount); i < max; ++i) {
        arguments.push_back(defaults[i]);
      }
      while (arguments.size() < argCount) {
        arguments.push_back(Value{});
      }
      call(result, arguments);
      arguments.clear();
    }
  };
}
