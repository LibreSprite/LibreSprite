// Aseprite
// Copyright (C) 2015-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/document_api.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ui_context.h"
#include "app/ui/document_view.h"
#include "doc/site.h"

#include "script/engine.h"
#include "script/engine_delegate.h"
#include "script/script_object.h"

#include <sstream>

class DudScriptObject : public script::InternalScriptObject {
public:
  void makeGlobal(const std::string& name) override {
    globalName = name;
  }
  std::string globalName;
};
static script::InternalScriptObject::Regular<DudScriptObject> dud("DudScriptObject");

namespace app {

class AppScriptObject : public script::ScriptObject {
public:
  inject<ScriptObject> m_pixelColor{"pixelColor"};
  std::vector<inject<ScriptObject>>  m_documents;

  AppScriptObject() {
    addProperty("activeFrameNumber", [this]{return updateSite() ? m_site.frame() : 0;})
      .doc("read-only. Returns the number of the currently active animation frame.");

    addProperty("activeLayerNumber", [this]{return updateSite() ? m_site.layerIndex() : 0;})
      .doc("read-only. Returns the number of the current layer.");

    addProperty("activeImage", []{return inject<ScriptObject>{"activeImage"}.get();})
      .doc("read-only, can be null. Returns the current layer/frame's image.");

    addProperty("activeSprite", []{return inject<ScriptObject>{"activeSprite"}.get();})
      .doc("read-only. Returns the currently active Sprite.");

    addProperty("activeDocument", []{return inject<ScriptObject>{"activeDocument"}.get();})
      .doc("read-only. Returns the currently active Document.");

    addProperty("pixelColor", [this]{return m_pixelColor.get();})
      .doc("read-only. Returns an object with functions for color conversion.");

    addProperty("version", []{return script::Value{VERSION};})
      .doc("read-only. Returns LibreSprite's current version as a string.");

    addMethod("documentation", &AppScriptObject::documentation)
      .doc("prints this text.");

    makeGlobal("app");
    init();
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

  ScriptObject* init() {
    if (!updateSite())
      return nullptr;

    int layerIndex = m_site.layerIndex();
    int frameIndex = m_site.frame();

    m_documents.emplace_back("DocumentScriptObject");

    auto sprite = m_documents.back()->get<ScriptObject*>("sprite");
    if (!sprite) {
      std::cout << "No sprite in document" << std::endl;
      return nullptr;
    }

    auto layer = sprite->call<ScriptObject*>("layer", layerIndex);
    if (!layer) {
      std::cout << "No layer in sprite" << std::endl;
      return nullptr;
    }

    return layer->call<ScriptObject*>("cel", frameIndex);
  }

  script::Value open(const std::string& fn) {
    if (fn.empty())
      return {};
    app::Document* oldDoc = UIContext::instance()->activeDocument();
    Command* openCommand = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;
    params.set("filename", fn.c_str());
    UIContext::instance()->executeCommand(openCommand, params);

    app::Document* newDoc = UIContext::instance()->activeDocument();
    if (newDoc == oldDoc)
      return {};

    m_documents.emplace_back("DocumentScriptObject");
    return inject<ScriptObject>{"activeSprite"}.get();
  }

  void App_exit() {
    Command* exitCommand = CommandsModule::instance()->getCommandByName(CommandId::Exit);
    UIContext::instance()->executeCommand(exitCommand);
  }

  doc::Site m_site;
};

static script::ScriptObject::Regular<AppScriptObject> reg("AppScriptObject", {"global"});

}
