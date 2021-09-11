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
    addProperty("layerCount", [this]{return (int) m_sprite->countLayers();})
      .doc("read-only. Returns the amount of layers in the sprite.");

    addProperty("filename", [this]{return doc()->filename();})
      .doc("read-only. Returns the file name of the sprite.");

    addProperty("width",
                [this]{return m_sprite->width();},
                [this](int width){
                  transaction().execute(new app::cmd::SetSpriteSize(m_sprite, width, m_sprite->height()));
                  return 0;
                })
      .doc("read+write. Returns and sets the width of the sprite.");

    addProperty("height",
                [this]{return m_sprite->height();},
                [this](int height){
                  transaction().execute(new app::cmd::SetSpriteSize(m_sprite, m_sprite->width(), height));
                  return 0;
                })
      .doc("read+write. Returns and sets the height of the sprite.");

    addProperty("colorMode", [this]{ return m_sprite->pixelFormat();})
      .doc("read-only. Returns the sprite's ColorMode.");

    addProperty("selection", [this]{ return this; })
      .doc("placeholder. Do not use.");

    addMethod("layer", &SpriteScriptObject::layer)
      .doc("allows you to access a given layer.")
      .docArg("layerNumber", "The number of they layer, starting with zero from the bottom.")
      .docReturns("a Layer object or null if invalid.");

    addMethod("commit", &SpriteScriptObject::commit)
      .doc("commits the current transaction.");

    addMethod("resize", &SpriteScriptObject::resize)
      .doc("resizes the sprite.")
      .docArg("width", "The new width.")
      .docArg("height", "The new height.");

    addMethod("crop", &SpriteScriptObject::crop)
      .doc("crops the sprite to the specified dimensions.")
      .docArg("x", "The left-most edge of the crop.")
      .docArg("y", "The top-most edge of the crop.")
      .docArg("width", "The width of the cropped area.")
      .docArg("height", "The height of the cropped area.");

    addMethod("save", &SpriteScriptObject::save)
      .doc("saves the sprite.");

    addMethod("saveAs", &SpriteScriptObject::saveAs)
      .doc("saves the sprite.")
      .docArg("fileName", "String. The new name of the file")
      .docArg("asCopy", "If true, the file is saved as a copy. Requires fileName to be specified.");

    addMethod("loadPalette", &SpriteScriptObject::loadPalette)
      .doc("loads a palette file.")
      .docArg("fileName", "The name of the palette file to load");
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
