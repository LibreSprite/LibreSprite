// Aseprite Scripting Library
// Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include <duk_config.h>
#include <duktape.h>

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/memory.h"
#include "script/engine.h"
#include "script/engine_delegate.h"

namespace {
  void on_fatal_handler(void* ctx, const char* msg) {
    throw base::Exception(msg);
  }

  void* on_alloc_function(void* udata, duk_size_t size) {
    if (size)
      return base_malloc(size);
    else
      return nullptr;
  }

  void* on_realloc_function(void* udata, void* ptr, duk_size_t size) {
    if (!ptr) {
      if (size)
        return base_malloc(size);
      else
        return nullptr;
    }
    else if (!size) {
      base_free(ptr);
      return nullptr;
    }
    else
      return base_realloc(ptr, size);
  }

  void on_free_function(void* udata, void* ptr) {
    if (ptr)
      base_free(ptr);
  }

// // TODO classes in modules isn't supported yet
// std::map<std::string, Module*> g_modules;

// duk_ret_t on_search_module(duk_context* ctx)
// {
  // const char* id = duk_get_string(ctx, 0);
  // if (!id)
  //   return 0;

  // auto it = g_modules.find(id);
  // if (it == g_modules.end()) {
  //   // TODO error module not found
  //   return 0;
  // }

  // Module* module = it->second;
  // Context ctxWrapper(ctx);
  // if (module->registerModule(ctxWrapper) > 0) {
  //   // Overwrite the 'exports' property of the module (arg 3)
  //   // with the object returned by registerModule()
  //   duk_put_prop_string(ctx, 3, "exports");
  // }

  // 0 means no source code
//   return 0;
// }
}

using namespace script;

class DukEngine : public Engine {
public:
  duk_hthread* m_handle;
  inject<EngineDelegate> m_delegate;

  DukEngine() :
    m_handle(duk_create_heap(&on_alloc_function,
                             &on_realloc_function,
                             &on_free_function,
                             (void*)this,
                             &on_fatal_handler))
    {
      InternalScriptObject::setDefault("DukScriptObject");
      // Set 'on_search_module' as the function to search modules with
      // require('modulename') on JavaScript.
      // duk_get_global_string(m_handle, "Duktape");
      // duk_push_c_function(m_handle, &on_search_module, 4);
      // duk_put_prop_string(m_handle, -2, "modSearch");
      // duk_pop(m_handle);
    }

  ~DukEngine() {
    duk_destroy_heap(m_handle);
  }

  bool raiseEvent(const std::vector<std::string>& event) override {
    try {
      duk_push_global_object(m_handle);
      duk_get_prop_string(m_handle, -1, "onEvent");
      if (duk_is_callable(m_handle, -1)) {
        for (auto& str : event) {
          duk_push_string(m_handle, str.c_str());
        }
        duk_call(m_handle, event.size());
        duk_pop(m_handle);// remove return value
      }
      duk_pop(m_handle);// remove global object
      // return eval("if (typeof onEvent === \"function\") onEvent(\"" + join(event, "\",\"") + "\");");
    } catch (const std::exception& ex) {
      return false;
    }
    return true;
  }

  bool eval(const std::string& code) override {
    bool success = true;
    try {
      initGlobals();
      if (duk_peval_string(m_handle, code.c_str()) != 0) {
        printLastResult();
        std::cout << "Error: [" << duk_safe_to_string(m_handle, -1) << "]" << std::endl;
        success = false;
      }

      if (m_printLastResult &&
          !duk_is_null_or_undefined(m_handle, -1)) {
        m_delegate->onConsolePrint(duk_safe_to_string(m_handle, -1));
      }

      duk_pop(m_handle);
    } catch (const std::exception& ex) {
      std::string err = "Error: ";
      err += ex.what();
      m_delegate->onConsolePrint(err.c_str());
      success = false;
      std::cout << "Error: [" << err << "]" << std::endl;
    }
    execAfterEval(success);
    return success;
  }
};

static Engine::Regular<DukEngine> registration("duk", {"js"});

#define HIDDEN(X)"\xff" X

class DukScriptObject : public InternalScriptObject {
  int m_refC = 0;

public:
  static Value getValue(duk_context* ctx, int id) {
    auto type = duk_get_type(ctx, id);
    if (type == DUK_TYPE_NUMBER) {
      return duk_get_number(ctx, id);
    } else if (type == DUK_TYPE_STRING) {
      return {duk_get_string(ctx, id)};
    } else if (type == DUK_TYPE_BOOLEAN) {
      return duk_get_boolean(ctx, id);
    } else if (type == DUK_TYPE_NULL) {
      return {};
    } else if (type == DUK_TYPE_OBJECT) {
      duk_size_t size = 0;
      void* buffer = duk_get_buffer_data(ctx, id, &size);
      if (buffer)
        return {buffer, size, false};
    } else if (type == DUK_TYPE_BUFFER) {
      duk_size_t size = 0;
      void* buffer = duk_get_buffer_data(ctx, id, &size);
      if (buffer)
        return {buffer, size, false};
    } else if (type == DUK_TYPE_UNDEFINED) {
      return {};
    }
    printf("Type: %d\n", type);
    return {};
  }

  static duk_ret_t callFunc(duk_context* ctx) {
    int argc = duk_get_top(ctx);
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, HIDDEN("func"));
    auto& func = *reinterpret_cast<script::Function*>(duk_get_pointer(ctx, -1));
    for (int i = 0; i < argc; ++i) {
      func.arguments.push_back(getValue(ctx, i));
    }
    func.result.makeUndefined();
    try {
        func();
    } catch (const ObjectDestroyedException&) {
      std::cout << "Object Destroyed Exception" << std::endl;
    }
    return returnValue(ctx, func.result);
  }

  static duk_ret_t dtorFunc(duk_context *ctx) {
    duk_get_prop_string(ctx, 0, HIDDEN("self"));
    auto self = reinterpret_cast<DukScriptObject*>(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    if (!self)
      return 0;

    // Mark as deleted
    duk_push_pointer(ctx, 0);
    duk_put_prop_string(ctx, 0, HIDDEN("self"));

    --self->m_refC;
    if (!self->m_refC && self->onRelease) {
      auto cb = std::move(self->onRelease);
      self->onRelease = nullptr;
      cb();
    }

    return 0;
  }

  static duk_ret_t returnValue(duk_context* ctx, const Value& value) {
    switch (value.type) {
    case Value::Type::UNDEFINED:
      return 0;

    case Value::Type::INT:
      duk_push_int(ctx, value);
      break;

    case Value::Type::DOUBLE:
      duk_push_number(ctx, value);
      break;

    case Value::Type::STRING:
      duk_push_string(ctx, value);
      break;

    case Value::Type::OBJECT:
      if (auto object = static_cast<ScriptObject*>(value)) {
        static_cast<DukScriptObject*>(object->getInternalScriptObject())->makeLocal();
      } else {
        duk_push_null(ctx);
      }
      break;

    case Value::Type::BUFFER: {
      auto& buffer = value.buffer();
      void* out = duk_push_buffer(ctx, buffer.size(), 0);
      memcpy(out, &buffer[0], buffer.size());
      break;
    }

    }

    return 1;
  }

  void pushFunctions() {
    auto handle = m_engine.get<DukEngine>()->m_handle;
    for (auto& entry : functions) {
      duk_push_c_function(handle, callFunc, DUK_VARARGS);
      duk_push_pointer(handle, &entry.second);
      duk_put_prop_string(handle, -2, HIDDEN("func"));
      duk_put_prop_string(handle, -2, entry.first.c_str());
    }

    m_refC++;
    duk_push_pointer(handle, this);
    duk_put_prop_string(handle, -2, HIDDEN("self"));
    // Store the destructor
    duk_push_c_function(handle, dtorFunc, 1);
    duk_set_finalizer(handle, -2);
  }

  static duk_ret_t getterFunc(duk_context* ctx) {
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, HIDDEN("func"));
    auto& prop = *reinterpret_cast<script::ObjectProperty*>(duk_get_pointer(ctx, -1));
    prop.getter();
    returnValue(ctx, prop.getter.result);
    return 1;
  }

  static duk_ret_t setterFunc(duk_context* ctx) {
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, HIDDEN("func"));
    auto& prop = *reinterpret_cast<script::ObjectProperty*>(duk_get_pointer(ctx, -1));
    prop.setter.arguments.emplace_back(getValue(ctx, 0));
    prop.setter();
    return 0;
  }

  void pushProperties() {
    auto handle = m_engine.get<DukEngine>()->m_handle;
    for (auto& entry : properties) {
      duk_push_string(handle, entry.first.c_str());
      auto& prop = entry.second;

      int idx = -2;
      duk_uint_t flags = 0;

      flags |= DUK_DEFPROP_HAVE_GETTER;
      duk_push_c_function(handle, getterFunc, 0);
      duk_push_pointer(handle, &prop);
      duk_put_prop_string(handle, -2, HIDDEN("func"));
      --idx;

      flags |= DUK_DEFPROP_HAVE_SETTER;
      duk_push_c_function(handle, setterFunc, 1);
      duk_push_pointer(handle, &prop);
      duk_put_prop_string(handle, -2, HIDDEN("func"));
      --idx;

      duk_def_prop(handle, idx, flags);
    }
  }

  void makeLocal() {
    auto handle = m_engine.get<DukEngine>()->m_handle;
    duk_push_object(handle);
    pushFunctions();
    pushProperties();
  }

  void makeGlobal(const std::string& name) override {
    auto handle = m_engine.get<DukEngine>()->m_handle;
    duk_push_global_object(handle);
    duk_push_object(handle);
    pushFunctions();
    pushProperties();
    duk_put_prop_string(handle, -2, name.c_str());
    duk_pop(handle);
  }
};

static InternalScriptObject::Regular<DukScriptObject> dukSO("DukScriptObject");
