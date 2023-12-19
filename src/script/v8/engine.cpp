// LibreSprite Scripting Library
// Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.


#include <cstring>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if SCRIPT_ENGINE_V8

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/memory.h"
#include "script/engine.h"
#include "script/engine_delegate.h"

#include <map>
#include <iostream>
#include <string>
#include <unordered_map>

#include <v8.h>
#include <libplatform/libplatform.h>

using namespace script;

template<typename Inner>
v8::Local<Inner> ToLocal(v8::Local<Inner> thing) {
  return thing;
}

template<typename Inner>
v8::Local<Inner> ToLocal(v8::MaybeLocal<Inner> thing) {
  if (thing.IsEmpty()) return {};
  return thing.ToLocalChecked();
}

template<typename T>
void Check(const T&){}

class V8Engine : public Engine {
public:
  inject<EngineDelegate> m_delegate;
  v8::Global<v8::Context> m_context;
  v8::Isolate* m_isolate = nullptr;

  V8Engine() {
    InternalScriptObject::setDefault("V8ScriptObject");
    initV8();
  }

  v8::Local<v8::Context> context() { return m_context.Get(m_isolate); }

  void initV8() {
    static std::unique_ptr<v8::Platform> m_platform;
    if (!m_platform) {
      // Conflicting documentation. Not sure if this is actually needed.
      // v8::V8::InitializeICUDefaultLocation(base::get_app_path().c_str());
      // v8::V8::InitializeExternalStartupData(base::get_app_path().c_str());
      v8::V8::InitializeICU();

      m_platform = v8::platform::NewDefaultPlatform();
      v8::V8::InitializePlatform(m_platform.get());
      v8::V8::Initialize();
    }

    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    m_isolate = v8::Isolate::New(params);

    v8::Isolate::Scope isolatescope(m_isolate);
    v8::HandleScope handle_scope(m_isolate);
    m_context = v8::Global<v8::Context>(m_isolate, v8::Context::New(m_isolate));
  }

  bool raiseEvent(const std::string& event) override {
    return eval("if (typeof onEvent === \"function\") onEvent(\"" + event + "\");");
  }

  bool eval(const std::string& code) override {
    bool success = true;
    try {
      v8::Isolate::Scope isolatescope(m_isolate);
      // Create a stack-allocated handle scope.
      v8::HandleScope handle_scope(m_isolate);

      // Enter the context for compiling and running the hello world script.
      v8::Context::Scope context_scope(context());

      v8::TryCatch trycatch(m_isolate);

      initGlobals();

      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source = ToLocal(v8::String::NewFromUtf8(m_isolate, code.c_str()));

      // Compile the source code.
      v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context(), source);
      // Run the script to get the result.
      v8::MaybeLocal<v8::Value> result;
      if (!script.IsEmpty()) {
          result = ToLocal(script)->Run(context());
      }

      if (result.IsEmpty()) {
        if (trycatch.HasCaught()) {
          v8::Local<v8::Value> exception = trycatch.Exception();
          auto trace = trycatch.StackTrace(context());

          v8::String::Utf8Value utf8(m_isolate, exception);
          m_delegate->onConsolePrint(*utf8);

          if (!trace.IsEmpty()){
            v8::String::Utf8Value utf8Trace(m_isolate, ToLocal(trace));
            m_delegate->onConsolePrint(*utf8Trace);
          }

          std::cout << "Error: [" << *utf8 << "]" << std::endl;
          success = false;
        }
      } else if (m_printLastResult) {
        v8::String::Utf8Value utf8(m_isolate, result.ToLocalChecked());
        m_delegate->onConsolePrint(*utf8);
      }
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

static Engine::Regular<V8Engine> registration("js", {"js"});

class V8ScriptObject : public InternalScriptObject {
  v8::Persistent<v8::Object> m_local;
public:

  ~V8ScriptObject() {
    if (!m_local.IsEmpty()) {
      m_local.ClearWeak();
      m_local.Reset();
    }
  }

  static Value getValue(v8::Isolate *isolate, v8::Local<v8::Value> local) {
    if (local->IsNullOrUndefined())
      return {};

    if (local->IsString()) {
      v8::String::Utf8Value utf8(isolate, local);
      return {*utf8};
    }

    if (local->IsNumber())
      return local.As<v8::Number>()->Value();

    if (local->IsUint32())
      return local.As<v8::Uint32>()->Value();

    if (local->IsInt32())
      return local.As<v8::Int32>()->Value();

    if (local->IsBoolean())
      return local.As<v8::Boolean>()->Value();

    if (local->IsUint8Array()){
      auto array = local.As<v8::Uint8Array>();
#if V8_MAJOR_VERSION > 7
      auto store = array->Buffer()->GetBackingStore();
#else
      auto storeObj = array->Buffer()->GetContents();
      auto store = &storeObj;
#endif
      return {
        store->Data(),
        store->ByteLength(),
        false
      };
    }

    v8::String::Utf8Value utf8(isolate, local->TypeOf(isolate));
    printf("Unknown type: [%s]\n", *utf8);

    return {};
  }

  static v8::Local<v8::Value> returnValue(v8::Isolate* isolate, const Value& value) {
    switch (value.type) {
    case Value::Type::UNDEFINED:
      return v8::Local<v8::Value>();

    case Value::Type::INT:
      return v8::Int32::New(isolate, value);

    case Value::Type::DOUBLE:
      return v8::Number::New(isolate, value);

    case Value::Type::STRING:
      return ToLocal(v8::String::NewFromUtf8(isolate, value));

    case Value::Type::OBJECT:
      if (auto object = static_cast<ScriptObject*>(value)) {
        return static_cast<V8ScriptObject*>(object->getInternalScriptObject())->makeLocal();
      }
      return {};

    case Value::Type::BUFFER: {
      auto& buffer = value.buffer();
      if (buffer.canSteal()) {
#if V8_MAJOR_VERSION > 7
        auto store = v8::ArrayBuffer::NewBackingStore(
          buffer.steal(),
          buffer.size(),
          +[](void* data, size_t length, void* deleter_data){
            delete[] static_cast<uint8_t*>(data);
          },
          nullptr
        );
        auto arrayBuffer = v8::ArrayBuffer::New(isolate, std::move(store));
        return v8::Uint8Array::New(arrayBuffer, 0, buffer.size());
#else
        auto arrayBuffer = v8::ArrayBuffer::New(isolate, buffer.steal(), buffer.size());
#endif
        return v8::Uint8Array::New(arrayBuffer, 0, buffer.size());

      } else {
        auto arrayBuffer = v8::ArrayBuffer::New(isolate, buffer.size());
#if V8_MAJOR_VERSION > 7
        std::memcpy(arrayBuffer->GetBackingStore()->Data(), buffer.data(), buffer.size());
#else
        std::memcpy(arrayBuffer->GetContents().Data(), buffer.data(), buffer.size());
#endif
        return v8::Uint8Array::New(arrayBuffer, 0, buffer.size());
      }
    }

    default:
      printf("Unknown return type: %d\n", int(value.type));
      break;
    }
    return {};
  }

  static void callFunc(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto data = args.Data().As<v8::External>();
    auto& func = *reinterpret_cast<script::Function*>(data->Value());

    for (int i = 0; i < args.Length(); i++) {
      func.arguments.push_back(getValue(isolate, args[i]));
    }

    func();

    args.GetReturnValue().Set(returnValue(isolate, func.result));
  }

  void pushFunctions(v8::Local<v8::Object>& object) {
    auto isolate = m_engine.get<V8Engine>()->m_isolate;
    auto context = m_engine.get<V8Engine>()->context();
    for (auto& entry : functions) {
      auto tpl = v8::FunctionTemplate::New(isolate, callFunc, v8::External::New(isolate, &entry.second));
      auto func = tpl->GetFunction(context).ToLocalChecked();
      Check(object->Set(context,
                  ToLocal(v8::String::NewFromUtf8(isolate, entry.first.c_str())),
                  func));
    }
  }

  void pushProperties(v8::Local<v8::Object>& object) {
    auto& isolate = m_engine.get<V8Engine>()->m_isolate;
    auto context = m_engine.get<V8Engine>()->context();

    for (auto& entry : properties) {
      auto getterTpl = v8::FunctionTemplate::New(isolate, callFunc, v8::External::New(isolate, &entry.second.getter));
      auto getter = getterTpl->GetFunction(context).ToLocalChecked();

      auto setterTpl = v8::FunctionTemplate::New(isolate, callFunc, v8::External::New(isolate, &entry.second.setter));
      auto setter = setterTpl->GetFunction(context).ToLocalChecked();

      v8::PropertyDescriptor descriptor(getter, setter);
      Check(object->DefineProperty(context,
                             ToLocal(v8::String::NewFromUtf8(isolate, entry.first.c_str())),
                             descriptor));
    }
  }

  v8::Local<v8::Object> makeLocal() {
    auto isolate = m_engine.get<V8Engine>()->m_isolate;
    if (!m_local.IsEmpty())
      return m_local.Get(isolate);
    auto local = v8::Object::New(isolate);
    pushFunctions(local);
    pushProperties(local);
    m_local.Reset(isolate, local);
    m_local.SetWeak(this, [](const auto& info) {
      reinterpret_cast<V8ScriptObject*>(info.GetParameter())->release();
    }, v8::WeakCallbackType::kParameter);
    return local;
  }

  void release() {
    m_local.ClearWeak();
    m_local.Reset();
    if (onRelease) {
      auto cb = std::move(onRelease);
      onRelease = nullptr;
      cb();
    }
  }

  void makeGlobal(const std::string& name) override {
    auto& isolate = m_engine.get<V8Engine>()->m_isolate;
    auto context = m_engine.get<V8Engine>()->context();
    Check(context->Global()->Set(context,
                                 ToLocal(v8::String::NewFromUtf8(isolate, name.c_str())),
                                 makeLocal()));
  }
};

static InternalScriptObject::Regular<V8ScriptObject> v8SO("V8ScriptObject");

#endif
