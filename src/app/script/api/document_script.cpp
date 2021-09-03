// LibreSprite
// Copyright (C) 2021 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/engine.h"
#include "app/document.h"
#include "app/ui_context.h"

class DocumentScriptObject : public script::ScriptObject {
public:
    DocumentScriptObject() {
        addProperty("sprite", [this]{return m_sprite.get();});
    }

    void* getWrapped() override {return m_doc;}

    Provides provides{this, "activeDocument"};
    doc::Document* m_doc{app::UIContext::instance()->activeDocument()};
    inject<ScriptObject> m_sprite{"SpriteScriptObject"};
};

static script::ScriptObject::Regular<DocumentScriptObject> reg("DocumentScriptObject");
