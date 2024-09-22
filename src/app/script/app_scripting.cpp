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

#include "app/app.h"
#include "app/document.h"
#include "app/script/app_scripting.h"
#include "app/task_manager.h"
#include "base/file_handle.h"
#include "base/path.h"
#include "base/range.h"
#include "base/string.h"
#include "base/trim_string.h"
#include "script/engine.h"
#include "script/engine_delegate.h"
#include "script/value.h"
#include "ui/keys.h"
#include "ui/message.h"
#include "ui/message_type.h"
#include "ui/widget.h"
#include "ui/manager.h"

#include <map>
#include <utility>
#include <string>
#include <string_view>
#include <fstream>

namespace {

inject<script::Engine> engine{nullptr};
std::string previousFileName;
bool wasInit{};

}

namespace app {
  std::string AppScripting::m_fileName;
  std::multimap<std::string, std::string> eventHooks;

  class EventListener : public ui::Widget {
  public:
    bool onProcessMessage(ui::Message* msg) override {
      auto event = ui::to_string(msg->type());
      auto range = base::range{eventHooks.equal_range(event)};
      if (range.empty())
        return false;

      std::vector<script::Value> args {
        event,
        static_cast<int>(msg->modifiers())
      };

      switch (msg->type()) {
      case ui::kKeyDownMessage:
      case ui::kKeyUpMessage:
        args.push_back(static_cast<int>(static_cast<ui::KeyMessage*>(msg)->unicodeChar()));
        args.push_back(static_cast<int>(static_cast<ui::KeyMessage*>(msg)->scancode()));
        break;

      case ui::kMouseDownMessage:
      case ui::kMouseUpMessage:
      case ui::kDoubleClickMessage:
      case ui::kMouseEnterMessage:
      case ui::kMouseLeaveMessage:
      case ui::kMouseMoveMessage:
      case ui::kSetCursorMessage:
      case ui::kMouseWheelMessage:
        args.push_back(static_cast<int>(static_cast<ui::MouseMessage*>(msg)->position().x));
        args.push_back(static_cast<int>(static_cast<ui::MouseMessage*>(msg)->position().y));
        args.push_back(static_cast<int>(static_cast<ui::MouseMessage*>(msg)->buttons()));
        args.push_back(static_cast<int>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().x));
        args.push_back(static_cast<int>(static_cast<ui::MouseMessage*>(msg)->wheelDelta().y));
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
    bool supported = false;
    auto extension = base::string_to_lower(base::get_file_extension(fullPath));
    for (auto& entry : script::Engine::getRegistry()) {
      if (entry.second.hasFlag(extension)) {
        supported = true;
        break;
      }
    }

    if (!supported)
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
    if (!wasInit) {
      wasInit = true;
      App::instance()->Exit.connect([]{
        engine = nullptr;
      });
    }

    // if there is no engine OR
    // the engine we have doesn't match the default in the registry,
    // inject a new one
    if (!engine || !script::Engine::getRegistry()[""].match(engine.get())) {
        bool printLast = engine && engine->getPrintLastResult();
        engine = inject<script::Engine>();
        if (engine && printLast)
            engine->getPrintLastResult();
    }
  }

  void AppScripting::raiseEvent(const std::string& fileName, const std::vector<script::Value> &event) {
    TaskManager::instance().delayed([=]{
      if ((engine && fileName == previousFileName) || evalFile(fileName)) {
        engine->raiseEvent(event);
      }
    });
  }

  bool AppScripting::eval(const std::string& code) {
    initEngine();
    if (engine) {
      return engine->eval(code);
    }
    inject<script::EngineDelegate>{}->onConsolePrint("No compatible scripting engine.");
    return false;
  }

  bool AppScripting::evalFile(const std::string& fileName) {
    m_fileName = fileName;
    std::cout << "Reading file " << fileName << std::endl;
    std::ifstream ifs(fileName);
    if (!ifs) {
      std::cout << "Could not open " << fileName << std::endl;
      return false;
    }

    auto extension = base::string_to_lower(base::get_file_extension(fileName));
    script::Engine::setDefault(extension, {extension});

    engine = nullptr;

    AppScripting instance;
    if (!instance.eval({std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()}))
      return false;

    engine->raiseEvent({"init"});

    previousFileName = fileName;
    return true;
  }

  void AppScripting::printLastResult() {
    if(engine)
      engine->printLastResult();
  }

}
