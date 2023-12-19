// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/modules/palettes.h"
#include "doc/object.h"
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
                  if (auto pal = palette())
                    return pal->size();
                  return 0;
                },
                [this](int s){
                  if (auto pal = palette()) {
                    pal->resize(s);
                    modify();
                  }
                  return s;
                });

    addFunction("get", [this](int i){
      if (auto pal = palette())
        return pal->getEntry(i);
      return doc::color_t{};
    });

    addMethod("set", &PaletteScriptObject::set);
  }

  doc::Palette* palette() {
    return handle<doc::Object, doc::Palette>();
  }

  void modify() {
      if (needIncrement)
          return;
      needIncrement = true;
      getEngine()->afterEval([=](bool success){
        auto pal = palette();
        if (pal) {
          pal->incrementVersion();
          app::set_current_palette(pal, true);
          ui::Manager::getDefault()->invalidate();
        }
        needIncrement = false;
      });
  }

  void set(int i){
        auto pal = palette();
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

  bool needIncrement = false;
};

static script::ScriptObject::Regular<PaletteScriptObject> celSO("PaletteScriptObject");
