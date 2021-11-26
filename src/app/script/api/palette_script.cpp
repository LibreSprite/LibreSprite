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
                [this]{return m_pal->size();},
                [this](int s){m_pal->resize(s); modify(); return s;});

    addFunction("get", [this](int i){return m_pal->getEntry(i);});

    addMethod("set", &PaletteScriptObject::set);
  }

  void modify() {
      if (needIncrement)
          return;
      needIncrement = true;
      m_engine->afterEval([=](bool success){
          m_pal->incrementVersion();
          app::set_current_palette(m_pal, true);
          ui::Manager::getDefault()->invalidate();
          needIncrement = false;
      });
  }

  void set(int i){
      if (i >= m_pal->size())
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
      m_pal->setEntry(i, c);
      modify();
  }

  void* getWrapped() override {return m_pal;}
  void setWrapped(void* pal) override {m_pal = static_cast<doc::Palette*>(pal);}

  bool needIncrement = false;
  doc::Palette* m_pal = nullptr;
  inject<script::Engine> m_engine;
};

static script::ScriptObject::Regular<PaletteScriptObject> celSO("PaletteScriptObject");
