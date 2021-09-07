// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/cmd/set_sprite_size.h"
#include "app/commands/commands.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/file/palette_file.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "doc/document_observer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "script/script_object.h"

class SpriteScriptObject : public script::ScriptObject {
  Provides provides{this, "activeSprite"};
  inject<ScriptObject> m_document{"activeDocument"};
  doc::Sprite* m_sprite;
  std::unordered_map<doc::Layer*, inject<ScriptObject>> m_layers;
  std::unique_ptr<app::Transaction> m_transaction;

public:
  SpriteScriptObject() : m_sprite{doc()->sprite()} {
    addProperty("layerCount", [this]{return (int) m_sprite->countLayers();});
    addProperty("filename", [this]{return doc()->filename();});
    addProperty("width",
                [this]{return m_sprite->width();},
                [this](int width){
                  transaction().execute(new app::cmd::SetSpriteSize(m_sprite, width, m_sprite->height()));
                  return 0;
                });
    addProperty("height",
                [this]{return m_sprite->height();},
                [this](int height){
                  transaction().execute(new app::cmd::SetSpriteSize(m_sprite, m_sprite->width(), height));
                  return 0;
                });
    addProperty("colorMode", [this]{ return m_sprite->pixelFormat();});
    addProperty("selection", [this]{ return this; });
    addMethod("layer", &SpriteScriptObject::layer);
    addMethod("commit", &SpriteScriptObject::commit);
    addMethod("resize", &SpriteScriptObject::resize);
    addMethod("crop", &SpriteScriptObject::crop);
    addMethod("save", &SpriteScriptObject::save);
    addMethod("saveAs", &SpriteScriptObject::saveAs);
    addMethod("loadPalette", &SpriteScriptObject::loadPalette);
  }

  ~SpriteScriptObject() {
    commit();
  }

  void* getWrapped() override {return m_sprite;}

  app::Document* doc() {
    return m_document->getWrapped<app::Document>();
  }

  app::Transaction& transaction() {
    if (!m_transaction) {
      m_transaction.reset(new app::Transaction(app::UIContext::instance(),
                                               "Script Execution",
                                               app::ModifyDocument));
    }
    return *m_transaction;
  }

  void commit() {
    for (auto& entry : m_layers) {
      entry.second->call("commit");
    }
    if (m_transaction) {
      m_transaction->commit();
      m_transaction.reset();
    }
  }

  script::ScriptObject* layer(int i) {
    auto layer = m_sprite->indexToLayer(doc::LayerIndex(i));
    if (!layer)
      return nullptr;
    auto it = m_layers.find(layer);
    if (it == m_layers.end()) {
      it = m_layers.emplace(layer, "LayerScriptObject").first;
      it->second->setWrapped(layer);
    }
    return it->second.get();
  }

  void resize(int w, int h) {
    app::DocumentApi api(doc(), transaction());
    api.setSpriteSize(m_sprite, w, h);
  }

  void crop(script::Value x, script::Value y, script::Value w, script::Value h){
    gfx::Rect bounds;
    commit();

    if (doc()->isMaskVisible())
      bounds = doc()->mask()->bounds();
    else
      bounds = m_sprite->bounds();

    if (x.type != script::Value::Type::UNDEFINED) bounds.x = x;
    if (y.type != script::Value::Type::UNDEFINED) bounds.y = y;
    if (w.type != script::Value::Type::UNDEFINED) bounds.w = w;
    if (h.type != script::Value::Type::UNDEFINED) bounds.h = h;

    if (!bounds.isEmpty()) {
      app::DocumentApi{doc(), transaction()}.cropSprite(m_sprite, bounds);
    }
  }

  void save() {
    commit();
    auto uiCtx = app::UIContext::instance();
    uiCtx->setActiveDocument(doc());
    auto saveCommand = app::CommandsModule::instance()->getCommandByName(app::CommandId::SaveFile);
    uiCtx->executeCommand(saveCommand);
  }

  void saveAs(const std::string& fileName, bool asCopy) {
    commit();
    if (fileName.empty()) asCopy = false;
    auto uiCtx = app::UIContext::instance();
    uiCtx->setActiveDocument(doc());
    auto commandName = asCopy ? app::CommandId::SaveFileCopyAs : app::CommandId::SaveFile;
    auto saveCommand = app::CommandsModule::instance()->getCommandByName(commandName);
    app::Params params;
    if (asCopy) params.set("filename", fileName.c_str());
    else if(!fileName.empty()) doc()->setFilename(fileName);
    uiCtx->executeCommand(saveCommand, params);
  }

  void loadPalette(const std::string& fileName){
    base::UniquePtr<doc::Palette> palette(app::load_palette(fileName.c_str()));
    if (palette) {
      // TODO Merge this with the code in LoadPaletteCommand
      doc()->getApi(transaction()).setPalette(m_sprite, 0, palette);
    }
  }
};

static script::ScriptObject::Regular<SpriteScriptObject> spriteSO("SpriteScriptObject");
