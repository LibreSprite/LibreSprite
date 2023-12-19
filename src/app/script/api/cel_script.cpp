// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "script/script_object.h"
#include "script/engine.h"

class CelScriptObject : public script::ScriptObject {
public:
  CelScriptObject() {
    addProperty("x",
                [this]{return cel()->x();},
                [this](int x){cel()->setPosition(x, cel()->y()); return x;});
    addProperty("y",
                [this]{return cel()->y();},
                [this](int y){cel()->setPosition(cel()->x(), y); return y;});
    addProperty("image", [this]{return getEngine()->getScriptObject(cel()->image());});
    addProperty("frame", [this]{return cel()->frame();});

    addFunction("setPosition", [this](int x, int y){
      cel()->setPosition(x, y);
      return this;
    });
  }

  doc::Cel* cel() {
    auto cel = handle<doc::Object, doc::Cel>();
    if (!cel)
      throw script::ObjectDestroyedException{};
    return cel;
  }
};

static script::ScriptObject::Regular<CelScriptObject> celSO("CelScriptObject");
