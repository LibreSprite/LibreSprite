// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// Web/Android stub for the child_process extension: the browser (and
// Android) cannot start processes, so the functions throw. The native
// build uses child_process_script.cpp instead.

#if defined(__EMSCRIPTEN__) || defined(ANDROID)

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include <string>

class ChildProcessExtension : public Extension {
public:
  ChildProcessExtension() {
    addFunction("__cpSpawn") = [](const std::string&, JSON::Value&) -> JSON::Value {
      throw std::runtime_error("child_process is not supported on this platform");
    };
    addFunction("__cpExec") = [](const std::string&, JSON::Value&, JSON::Value&) -> JSON::Value {
      throw std::runtime_error("child_process is not supported on this platform");
    };
  }

  std::string init(const std::string& language, JSON::Value&) override {
    if (language != "js")
      return "";
    return R"(
globalThis.child_process = {};
child_process.spawn = function() {
    throw new Error("child_process is not supported on this platform");
};
child_process.exec = function() {
    return Promise.reject(new Error("child_process is not supported on this platform"));
};
)";
  }
};

static di::provide<Extension, ChildProcessExtension> cpExt{"child_process"};

#endif // __EMSCRIPTEN__ || ANDROID
