// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/console.h"
#include "app/app.h"
#include "ui/manager.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

namespace {

  // Routes text to app::Console (the DevConsole panel in GUI mode; stdout
  // fallback when no UI is available). The Console is created lazily on the
  // first print so m_withUI reflects the UI state at that point (not at
  // program startup, before the Manager exists). Replaces the old
  // script::EngineDelegate indirection (console_delegate.cpp, now removed).
  // In GUI mode the text is ALSO written to stdout so headless `--script`
  // observation / logging still works alongside the DevConsole panel.
  void consolePrint(const std::string& text) {
    static app::Console console;
    console.printf("%s\n", text.c_str());
    if (app::App::instance() && app::App::instance()->isGui() &&
        ui::Manager::getDefault() && ui::Manager::getDefault()->getDisplay()) {
      std::cout << text << std::endl;
    }
  }

  std::string formatValue(JSON::Value& v) {
    if (v.isString())
      return v.string();
    if (v.isNumber()) {
      double d = v.number();
      if (std::floor(d) == d && std::fabs(d) < 9007199254740992.0)
        return std::to_string(static_cast<long long>(d));
      std::ostringstream os;
      os << d;
      return os.str();
    }
    return v.toString();
  }

  std::string joinArgs(JSON::Array& args) {
    std::string out;
    bool first = true;
    for (auto& arg : args) {
      if (!first)
        out += " ";
      first = false;
      out += formatValue(arg);
    }
    return out;
  }

} // namespace

// `console` is a global object of variadic functions. Delta class methods are
// not variadic (a raw method lambda receives a single arg, not the full array),
// so the variadic functions are registered as globals and the `console` object
// is assembled from them in the boot script.
class ConsoleExtension : public Extension {
public:
  ConsoleExtension() {
    addVarArgFunction("__consoleLog") = [](JSON::Array& args) -> JSON::Value {
      consolePrint(joinArgs(args));
      return {};
    };
    addVarArgFunction("__consoleVerbose") = [](JSON::Array& args) -> JSON::Value {
      consolePrint(joinArgs(args));
      return {};
    };
    addFunction("__consoleAssert") = [](JSON::Value& cond, JSON::Value& msg) -> JSON::Value {
      if (!static_cast<bool>(cond))
        consolePrint(msg.isUndefined() ? std::string{} : msg.string());
      return {};
    };
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return "globalThis.console = { log: __consoleLog, verbose: __consoleVerbose, assert: __consoleAssert };";
  }
};

static di::provide<Extension, ConsoleExtension> x{"console"};
