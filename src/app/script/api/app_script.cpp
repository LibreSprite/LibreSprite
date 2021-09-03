// Aseprite
// Copyright (C) 2015-2016  David Capello
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/engine.h"

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

namespace app {

class AppScriptObject : public script::ScriptObject {
public:
  inject<ScriptObject> m_pixelColor{"pixelColor"};
  std::vector<inject<ScriptObject>>  m_documents;

  AppScriptObject() {
    addProperty("activeFrameNumber", [this]{return updateSite() ? m_site.frame() : 0;});
    addProperty("activeLayerNumber", [this]{return updateSite() ? m_site.layerIndex() : 0;});
    addProperty("activeImage", []{return inject<ScriptObject>{"activeImage"}.get();});
    addProperty("activeSprite", []{return inject<ScriptObject>{"activeSprite"}.get();});
    addProperty("activeDocument", []{return inject<ScriptObject>{"activeDocument"}.get();});
    addProperty("pixelColor", [this]{return m_pixelColor.get();});
    addProperty("version", []{return script::Value{VERSION};});
    makeGlobal("app");
    init();
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
