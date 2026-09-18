// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "app/cmd/set_sprite_size.h"
#include "app/cmd/add_frame_tag.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/file/palette_file.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "doc/document.h"
#include "doc/frame_tag.h"
#include "doc/frame_tags.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include <memory>
#include <stdexcept>
#include <string>

// `Sprite` is a stateless *proxy*: every access re-resolves the active
// document's sprite (mirroring the old `AppScriptObject::updateSite()`), so
// the `sprite` global always tracks the active document rather than a
// snapshot. Sub-objects (`layer`, `palette`, `cel`, `image`) are wrapped
// snapshots of the specific doc objects.
namespace {
  app::Document* activeDocument() {
    return app::UIContext::instance()->activeDocument();
  }
  doc::Sprite* activeSprite() {
    auto* doc = activeDocument();
    if (!doc)
      throw std::runtime_error{"No active document"};
    return doc->sprite();
  }
} // namespace

class SpriteExtension : public Extension {
public:
  SpriteExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, SpriteSite>("Sprite");
    clazz.setConstructor() = []() -> std::shared_ptr<SpriteSite> {
      static std::shared_ptr<SpriteSite> site = std::make_shared<SpriteSite>();
      return site;
    };

    clazz.addGetter("layerCount") = [](SpriteSite&) -> JSON::Value {
      return (double)activeSprite()->countLayers();
    };

    clazz.addGetter("tags") = [](SpriteSite&) -> JSON::Value {
      auto tags = std::make_shared<JSON::Array>();
      for (auto* tag : activeSprite()->frameTags())
        tags->push_back(JSON::makeNative(wrap(tag)));
      return tags;
    };

    clazz.addMethod("addTag") = [](SpriteSite&, double from, double to) -> JSON::Value {
      auto* spr = activeSprite();
      if (from < 0 || to < from || to >= spr->totalFrames())
        throw std::runtime_error{"Frame tag range is outside the sprite frames"};

      auto* tag = new doc::FrameTag((doc::frame_t)from, (doc::frame_t)to);
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::AddFrameTag(spr, tag));
      tx.commit();
      return JSON::makeNative(wrap(tag));
    };

    clazz.addMethod("removeTag") = [](SpriteSite&, JSON::Value& value) -> JSON::Value {
      if (!value.isNative())
        throw std::runtime_error{"removeTag() requires a FrameTag"};
      auto& native = value.native();
      if (native.second != typeid(void) && native.second != typeid(doc::FrameTag))
        throw std::runtime_error{"removeTag() requires a FrameTag"};
      auto* rawTag = static_cast<doc::FrameTag*>(native.first.get());
      if (!rawTag)
        throw std::runtime_error{"removeTag() requires a FrameTag"};
      std::shared_ptr<doc::FrameTag> tag(native.first, rawTag);
      auto* spr = activeSprite();
      if (tag->owner() != &spr->frameTags())
        throw std::runtime_error{"FrameTag does not belong to the active sprite"};

      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::RemoveFrameTag(spr, tag.get()));
      tx.commit();
      return {};
    };

    clazz.addGetter("filename") = [](SpriteSite&) -> JSON::Value {
      return std::string{activeSprite()->document()->filename()};
    };

    clazz.addGetter("width") = [](SpriteSite&) -> JSON::Value {
      return (double)activeSprite()->width();
    };
    clazz.addSetter("width") = [](SpriteSite&, JSON::Value& v) {
      auto* spr = activeSprite();
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetSpriteSize(spr, static_cast<int>(v), spr->height()));
      tx.commit();
    };

    clazz.addGetter("height") = [](SpriteSite&) -> JSON::Value {
      return (double)activeSprite()->height();
    };
    clazz.addSetter("height") = [](SpriteSite&, JSON::Value& v) {
      auto* spr = activeSprite();
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetSpriteSize(spr, spr->width(), static_cast<int>(v)));
      tx.commit();
    };

    clazz.addGetter("colorMode") = [](SpriteSite&) -> JSON::Value {
      return (double)activeSprite()->pixelFormat();
    };

    clazz.addGetter("selection") = [](SpriteSite&) -> JSON::Value {
      return JSON::makeNative(std::make_shared<SelectionSite>());
    };

    clazz.addGetter("palette") = [](SpriteSite&) -> JSON::Value {
      return JSON::makeNative(wrap(activeSprite()->palette(0)));
    };

    clazz.addMethod("layer") = [](SpriteSite&, double i) -> JSON::Value {
      auto* layer = activeSprite()->indexToLayer(doc::LayerIndex((int)i));
      return JSON::makeNative(wrap(layer));
    };

    // In the proxy model each mutation commits its own transaction, so there
    // is no persistent transaction to commit here. Kept as a no-op for
    // backward compatibility with the old API.
    clazz.addMethod("commit") = [](SpriteSite&) -> JSON::Value {
      return {};
    };

    clazz.addMethod("resize") = [](SpriteSite&, double w, double h) -> JSON::Value {
      auto* spr = activeSprite();
      app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
      tx.execute(new app::cmd::SetSpriteSize(spr, (int)w, (int)h));
      tx.commit();
      return {};
    };

    // The old implementation was disabled (its body was commented out); kept
    // as a no-op for API compatibility.
    clazz.addMethod("crop") = [](SpriteSite&, double, double, double, double) -> JSON::Value {
      return {};
    };

    clazz.addMethod("save") = [](SpriteSite&) -> JSON::Value {
      auto* doc = activeDocument();
      auto* uiCtx = app::UIContext::instance();
      uiCtx->setActiveDocument(doc);
      auto* saveCommand = app::CommandsModule::instance()->getCommandByName(app::CommandId::SaveFile);
      uiCtx->executeCommand(saveCommand);
      return {};
    };

    clazz.addMethod("saveAs") = [](SpriteSite&, const std::string& fileName, bool asCopy) -> JSON::Value {
      auto* doc = activeDocument();
      auto* uiCtx = app::UIContext::instance();
      uiCtx->setActiveDocument(doc);
      auto commandName = asCopy ? app::CommandId::SaveFileCopyAs : app::CommandId::SaveFile;
      auto* saveCommand = app::CommandsModule::instance()->getCommandByName(commandName);
      app::Params params;
      if (asCopy) {
        params.set("filename", fileName.c_str());
      } else if (!fileName.empty()) {
        doc->setFilename(fileName);
      }
      uiCtx->executeCommand(saveCommand, params);
      return {};
    };

    clazz.addMethod("loadPalette") = [](SpriteSite&, const std::string& fileName) -> JSON::Value {
      auto* doc = activeDocument();
      auto palette = app::load_palette(fileName.c_str());
      if (palette) {
        app::Transaction tx(app::UIContext::instance(), "Script Execution", app::ModifyDocument);
        doc->getApi(tx).setPalette(activeSprite(), 0, palette.get());
        tx.commit();
      }
      return {};
    };
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return "globalThis.sprite = new Sprite();";
  }
};

static di::provide<Extension, SpriteExtension> x{"sprite"};
