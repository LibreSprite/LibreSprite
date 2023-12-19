// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "doc/document.h"
#include "script/engine.h"
#include "app/document.h"
#include "app/ui_context.h"

class DocumentScriptObject : public script::ScriptObject {
public:
  DocumentScriptObject() {
    addProperty("sprite", [this]{return getEngine()->getScriptObject(doc()->sprite());});
  }

  doc::Document* doc() {
    auto doc = handle<doc::Object, doc::Document>();
    if (!doc)
      throw script::ObjectDestroyedException{};
    return doc;
  }
};

static script::ScriptObject::Regular<DocumentScriptObject> reg(typeid(doc::Document*).name());
