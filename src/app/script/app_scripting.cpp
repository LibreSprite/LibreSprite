// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "delta/Extension.hpp"
#include "delta/Interpreter.hpp"
#include "app/app.h"
#include "app/document.h"
#include "app/script/app_scripting.h"
#include "app/task_manager.h"
#include "base/file_handle.h"
#include "base/injection.h"
#include "base/path.h"
#include "base/range.h"
#include "base/string.h"
#include "base/trim_string.h"
#include "di.hpp"
#include "ui/keys.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_type.h"
#include "ui/widget.h"

#include <map>
#include <utility>
#include <string>
#include <string_view>
#include <fstream>

namespace {

std::unordered_set<std::string> loadedScripts;
std::shared_ptr<Interpreter> engine;
std::vector<std::shared_ptr<Extension>> extensions;
JSON::Value LS;

}

namespace app {
  std::multimap<std::string, std::string> eventHooks;

  class EventListener : public ui::Widget {
  public:
    bool onProcessMessage(ui::Message* msg) override {
      auto event = ui::to_string(msg->type());
      auto range = base::range{eventHooks.equal_range(event)};
      if (range.empty())
        return false;

      JSON::Value args;
      args.push_back(event);
      args.push_back(static_cast<double>(msg->modifiers()));

      switch (msg->type()) {
      case ui::kKeyDownMessage:
      case ui::kKeyUpMessage:
        args.push_back(static_cast<double>(static_cast<ui::KeyMessage*>(msg)->unicodeChar()));
        args.push_back(static_cast<double>(static_cast<ui::KeyMessage*>(msg)->scancode()));
        break;

      case ui::kMouseDownMessage:
      case ui::kMouseUpMessage:
      case ui::kDoubleClickMessage:
      case ui::kMouseEnterMessage:
      case ui::kMouseLeaveMessage:
      case ui::kMouseMoveMessage:
      case ui::kSetCursorMessage:
      case ui::kMouseWheelMessage:
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->position().x));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->position().y));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->buttons()));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().x));
        args.push_back(static_cast<double>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().y));
        break;

      default:
        break;
      }

      for (auto& entry : range)
        AppScripting::raiseEvent(entry.second, args);

      return false;
    }
  };

  std::unique_ptr<EventListener> listener;

  void AppScripting::clearEventHooks() {
    eventHooks.clear();
    if (listener) {
      ui::Manager::getDefault()->removeMessageListener(listener.get());
      listener.reset();
    }
  }

  bool AppScripting::scanScript(const std::string& fullPath) {
    auto extension = base::string_to_lower(base::get_file_extension(fullPath));
    if (extension != "js")
      return false;

    std::ifstream file{fullPath};

    if (!file)
      return false;

    std::string line;
    while (std::getline(file, line)) {
      base::trim_string(line, line);
      auto parts = base::split(line, ' ');

      for (auto& part : parts)
        base::trim_string(part, part);

      if (parts.empty() || parts[0].size() < 3 || parts[0].substr(0, 3) != "///")
        break;

      base::trim_string(parts[0].substr(3), parts[0]);
      if (parts[0].empty()) {
        parts.erase(parts.begin());
        if (parts.empty())
          break;
      }

      auto key = base::string_to_lower(parts[0]);
      parts.erase(parts.begin());

      if (key == "listen") {
        if (!listener) {
          listener.reset(new EventListener());
          ui::Manager::getDefault()->addMessageListener(listener.get());
        }
        for (auto& event : parts) {
          eventHooks.insert({event, fullPath});
        }
      }
    }

    return true;
  }

  void AppScripting::initEngine() {
    if (engine)
      return;

    App::instance()->Exit.connect([]{
      extensions.clear();
      engine.reset();
    });

    engine = di::inject<Interpreter>("js");

    LS["version"] = VERSION;
    LS["package"] = PACKAGE;
    engine->addGlobalValue("LS", LS);

    extensions = di::injectAll<Extension>();
    for (auto& ext : extensions)
      addExtension(*ext);
  }

  void AppScripting::addExtension(Extension& ext) {
    for (auto& [name, func] : ext.getFunctions())
      engine->addAPIFunction(name, func);
    for (auto& [className, clazz] : ext.getClasses()) {
      auto& cls = engine->addClass(className, clazz->getBaseType(), clazz->getDerivedType(), clazz->getConstructor());
      for (auto& [name, method] : clazz->getMethods()) {
        cls.addMethod(name, method);
      }
      for (auto& [name, getset] : clazz->getGetSet()) {
        cls.addGetSet(name, getset.get, getset.set);
      }
    }
    auto boot = ext.init("js", LS);
    if (!boot.empty()) {
      try {
        engine->eval(boot, typeid(ext).name());
      } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to run boot script for extension " << typeid(ext).name() << ": " << e.what() << std::endl;
      }
    }
  }

  void AppScripting::engineRaiseEvent(script::Value& event) {
  }

  void AppScripting::raiseEvent(const std::string& fileName, script::Value& event) {
    TaskManager::instance().delayed([=]() mutable {
      if (evalFile(fileName)) {
          engineRaiseEvent(event);
      }
    });
  }

  bool AppScripting::eval(const std::string& code, const std::string& path) {
    initEngine();
    if (engine) {
        loadedScripts.insert(path);
        return engine->eval(code, path).boolean();
    }
    return false;
  }

  bool AppScripting::evalFile(const std::string& fileName) {
    if (loadedScripts.find(fileName) != loadedScripts.end())
        return true;
    std::cout << "Reading file " << fileName << std::endl;
    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cout << "Could not open " << fileName << std::endl;
      return false;
    }

    if (!eval({std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()}, fileName))
      return false;

    JSON::Value args;
    args.push_back("init");
    engineRaiseEvent(args);
    return true;
  }

  void AppScripting::printLastResult() {
    // if(engine)
    //   engine->printLastResult();
  }

}
