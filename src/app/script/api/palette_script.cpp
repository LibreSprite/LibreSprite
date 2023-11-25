// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/modules/palettes.h"
#include "doc/palette.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "script/script_object.h"
#include "script/engine.h"

class PaletteScriptObject : public script::ScriptObject {
public:
  PaletteScriptObject() {
    addProperty("length",
                [this]{
                  if (auto pal = m_pal.lock())
                    return pal->size();
                  return 0;
                },
                [this](int s){
                  if (auto pal = m_pal.lock()) {
                    pal->resize(s);
                    modify();
                  }
                  return s;
                });

    addFunction("get", [this](int i){
      if (auto pal = m_pal.lock())
        return pal->getEntry(i);
      return doc::color_t{};
    });

    addMethod("set", &PaletteScriptObject::set);
  }

  void modify() {
      if (needIncrement)
          return;
      needIncrement = true;
      m_engine->afterEval([=](bool success){
        auto pal = m_pal.lock();
        if (pal) {
          pal->incrementVersion();
          app::set_current_palette(pal.get(), true);
          ui::Manager::getDefault()->invalidate();
        }
        needIncrement = false;
      });
  }

  void set(int i){
    auto pal = m_pal.lock();
    if (!pal)
      return;
    if (i >= pal->size())
      return;
    auto& args = script::Function::varArgs();
    int c;
    if (args.size() == 2) {
      c = args[1];
    } else if(args.size() == 4) {
      c = doc::rgba(args[1], args[2], args[3], 0xFF);
    } else if(args.size() == 5) {
      c = doc::rgba(args[1], args[2], args[3], args[4]);
    } else
      return;
    pal->setEntry(i, c);
    modify();
  }

  void* getWrapped() override {return m_pal.lock().get();}
  void setWrapped(void* pal) override {
    if (pal) {
      m_pal = std::static_pointer_cast<doc::Palette>(static_cast<doc::Object*>(pal)->shared_from_this());
    } else {
      m_pal.reset();
    }
  }

  bool needIncrement = false;
  std::weak_ptr<doc::Palette> m_pal;
  inject<script::Engine> m_engine;
};

static script::ScriptObject::Regular<PaletteScriptObject> celSO("PaletteScriptObject");
