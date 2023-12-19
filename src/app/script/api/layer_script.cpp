// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/script_object.h"
#include "doc/layer.h"
#include "doc/cel.h"
#include "script/engine.h"

class LayerScriptObject : public script::ScriptObject {
public:
  LayerScriptObject() {
    addProperty("name",
                [this]{return layer()->name();},
                [this](const std::string& name){
                  layer()->setName(name);
                  return name;
                })
      .doc("read+write. The name of the layer.");

    addProperty("isImage", [this]{return layer()->isImage();})
      .doc("read-only. Returns true if the layer is an image, false if it is a folder.");

    addProperty("isBackground", [this]{return layer()->isBackground();})
      .doc("read-only. Returns true if the layer is a background layer.");

    addProperty("isTransparent", [this]{return layer()->isTransparent();})
      .doc("read-only. Returns true if the layer is a non-background image layer.");

    addProperty("isVisible",
                [this]{return layer()->isVisible();},
                [this](bool i){
                  layer()->setVisible(i);
                  return i;
                })
      .doc("read+write. Gets/sets whether the layer is visible or not.");

    addProperty("isEditable",
                [this]{return layer()->isEditable();},
                [this](bool i){
                  layer()->setEditable(i);
                  return i;
                })
      .doc("read+write. Gets/sets whether the layer is editable (unlocked) or not (locked).");

    addProperty("isMovable", [this]{return layer()->isMovable();})
      .doc("read-only. Returns true if the layer is movable.");

    addProperty("isContinuous", [this]{return layer()->isContinuous();})
      .doc("read-only. Prefer to link cels when the user copies them.");

    addProperty("flags", [this]{return (int) layer()->flags();})
      .doc("read-only. Returns all flags OR'd together as an int");

    addProperty("celCount", [this]{
      return layer()->isImage() ? static_cast<doc::LayerImage*>(layer())->getCelsCount() : 0;
    }).doc("read-only. Returns the number of cels.");

    addFunction("cel", [this](int i){
      return getEngine()->getScriptObject(layer()->cel(i).get());
    }).doc("retrieves a Cel")
      .docArg("index", "The number of the Cel")
      .docReturns("A Cel object or null if an invalid index is passed");
  }

  doc::Layer* layer() {
    return handle<doc::Object, doc::Layer>();
  }
};

static script::ScriptObject::Regular<LayerScriptObject> layerSO("LayerScriptObject");
