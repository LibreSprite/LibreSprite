// LibreSprite
// Copyright (C) 2015-2016  David Capello
// Copyright (C) 2023 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_api.h"
#include "base/launcher.h"
#include "app/modules/editors.h"
#include "app/script/app_scripting.h"
#include "app/task_manager.h"
#include "app/ui/dialog.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/site.h"
#include "script/engine.h"
#include "script/engine_delegate.h"
#include "script/script_object.h"
#include "ui/widget.h"

#include <sstream>

class DudScriptObject : public script::InternalScriptObject {
public:
  void makeGlobal(const std::string& name) override {
    globalName = name;
  }
  std::string globalName;
};
static script::InternalScriptObject::Regular<DudScriptObject> dud("DudScriptObject");

namespace dialog {
ui::Widget* getDialogById(const std::string&);
}

namespace app {

class AppScriptObject : public script::ScriptObject {
public:
  inject<ScriptObject> m_pixelColor{"pixelColor"};
  inject<ScriptObject> m_command{"command"};

  AppScriptObject() {
    addProperty("activeFrameNumber", [this]{return updateSite() ? m_site.frame() : 0;})
      .doc("read-only. Returns the number of the currently active animation frame.");

    addProperty("activeLayerNumber", [this]{return updateSite() ? m_site.layerIndex() : 0;})
      .doc("read-only. Returns the number of the current layer.");

    addProperty("activeImage", [this]{
        return getEngine()->getScriptObject(app::current_editor ? app::current_editor->getSite().image() : nullptr);
    }).doc("read-only, can be null. Returns the current layer/frame's image.");

    addProperty("activeSprite", [this]{
        return getEngine()->getScriptObject(app::current_editor ? app::current_editor->getSite().sprite() : nullptr);
    }).doc("read-only. Returns the currently active Sprite.");

    addProperty("activeDocument", [this]{
        return getEngine()->getScriptObject(app::current_editor ? app::current_editor->getSite().document() : nullptr);
    }).doc("read-only. Returns the currently active Document.");

    addProperty("pixelColor", [this]{return m_pixelColor.get();})
      .doc("read-only. Returns an object with functions for color conversion.");

    addProperty("command", [this]{return m_command.get();})
      .doc("read-only. Returns an object with functions for running commands.");

    addProperty("version", []{return script::Value{VERSION};})
      .doc("read-only. Returns LibreSprite's current version as a string.");

    addProperty("platform", []() -> std::string {
      #ifdef EMSCRIPTEN
      return "emscripten";
      #elif _WIN32
      return "windows";
      #elif __APPLE__
      return "macos";
      #elif ANDROID
      return "android";
      #else
      return "linux";
      #endif
    }).doc("read-only. Returns one of: emscripten, windows, macos, android, linux.");

    addMethod("documentation", &AppScriptObject::documentation)
      .doc("Prints this text.");

    addMethod("createDialog", &AppScriptObject::createDialog)
      .doc("Creates a dialog window");

    addMethod("yield", &AppScriptObject::yield)
      .doc("Schedules a yield event on the next frame")
      .docArg("event", "Name of the event to be raised. The default is yield.");

    addMethod("open", &AppScriptObject::open)
      .doc("Opens a document for editing");

    addMethod("launch", &AppScriptObject::launch);

    addMethod("redraw", &AppScriptObject::redraw);

    makeGlobal("app");
  }

  void redraw() {
    ui::Manager::getDefault()->invalidate();
  }

  void yield(const std::string& event, int cycles) {
    auto fileName = app::AppScripting::getFileName();
    TaskManager::instance().delayed([=] {
      if (cycles > 0) {
        yield(event, cycles - 1);
        return;
      }
      app::AppScripting::raiseEvent(fileName, {event.empty() ? "yield" : event});
    });
  }

  ScriptObject* createDialog(const std::string& id) {
    auto dialog = getEngine()->create<ui::Dialog>();
    if (!dialog)
      return nullptr;

    if (!id.empty())
      dialog->set("id", id);

    return dialog;
  }

  void documentation() {
    std::stringstream out;
    if (!this->get("activeDocument")) {
      return;
    }

    auto& internalRegistry = script::InternalScriptObject::getRegistry();
    auto originalDefault = internalRegistry[""];
    script::InternalScriptObject::setDefault("DudScriptObject");

    for (auto& entry : script::ScriptObject::getRegistry()) {
      if (entry.first.empty())
        continue;
      inject<ScriptObject> so{entry.first};
      auto internal = dynamic_cast<DudScriptObject*>(so->getInternalScriptObject());
      if (!internal)
        continue;

      out << "# ";
      if (!internal->globalName.empty())
        out << "global " << internal->globalName << " ";

      std::string className = entry.first;
      auto dot = className.rfind("ScriptObject");
      if (dot != std::string::npos)
        className.resize(dot);

      out << "[class " << className << "]" << std::endl;

      if (internal->properties.empty()) {
        out << "## No Properties." << std::endl;
      } else {
        out << "## Properties: " << std::endl;
        for (auto& propEntry : internal->properties) {
          auto& prop = propEntry.second;
          out << "   - `" << propEntry.first << "`: " << prop.docStr << std::endl;
        }
      }

      out << std::endl;

      if (internal->functions.empty()) {
        out << "## No Methods." << std::endl;
      } else {
        out << "## Methods: " << std::endl;
        for (auto& funcEntry : internal->functions) {
          auto& func = funcEntry.second;
          out << "   - `" << funcEntry.first << "(";
          bool first = true;
          for (auto& arg : func.docArgs) {
            if (!first) out << ", ";
            first = false;
            out << arg.name;
          }
          out << ")`: " << std::endl;

          for (auto& arg : func.docArgs) {
            out << "     - " << arg.name << ": " << arg.docStr << std::endl;
          }

          out << "      returns: " << func.docReturnsStr << std::endl;

          if (!func.docStr.empty())
            out << "      " << func.docStr << std::endl;

          out << std::endl;
        }
      }
      out << std::endl << std::endl;
    }

    std::cout << out.str() << std::endl;

    // inject<script::EngineDelegate>{}->onConsolePrint(out.str().c_str());

    internalRegistry[""] = originalDefault;
  }

  bool updateSite() {
    app::Document* doc = UIContext::instance()->activeDocument();
    app::DocumentView* m_view = UIContext::instance()->getFirstDocumentView(doc);
    if (!m_view)
      return false;
    m_view->getSite(&m_site);
    return true;
  }

  script::Value open(const std::string& fn) {
    if (fn.empty())
      return {};
    auto oldDoc = static_cast<doc::Document*>(UIContext::instance()->activeDocument());
    Command* openCommand = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;
    params.set("filename", fn.c_str());
    UIContext::instance()->executeCommand(openCommand, params);
    auto newDoc = static_cast<doc::Document*>(UIContext::instance()->activeDocument());
    if (newDoc == oldDoc)
      return {};
    return getEngine()->getScriptObject(newDoc);
  }

  bool launch(const std::string& cmd) {
    return base::launcher::open_file(cmd);
  }

  void App_exit() {
    Command* exitCommand = CommandsModule::instance()->getCommandByName(CommandId::Exit);
    UIContext::instance()->executeCommand(exitCommand);
  }

  std::unordered_map<ui::Widget*, ScriptObject*> m_dialogScriptObjects;
  doc::Site m_site;
};

static script::ScriptObject::Regular<AppScriptObject> reg("AppScriptObject", {"global"});

}
