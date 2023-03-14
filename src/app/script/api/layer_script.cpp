// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/script_object.h"
#include "doc/layer.h"

class LayerScriptObject : public script::ScriptObject {
public:
  LayerScriptObject() {
    addProperty("name",
                [this]{return m_layer->name();},
                [this](const std::string& name){
                  m_layer->setName(name);
                  return name;
                })
      .doc("read+write. The name of the layer.");

    addProperty("isImage", [this]{return m_layer->isImage();})
      .doc("read-only. Returns true if the layer is an image, false if it is a folder.");

    addProperty("isBackground", [this]{return m_layer->isBackground();})
      .doc("read-only. Returns true if the layer is a background layer.");

    addProperty("isTransparent", [this]{return m_layer->isTransparent();})
      .doc("read-only. Returns true if the layer is a non-background image layer.");

    addProperty("isVisible",
                [this]{return m_layer->isVisible();},
                [this](bool i){
                  m_layer->setVisible(i);
                  return i;
                })
      .doc("read+write. Gets/sets whether the layer is visible or not.");

    addProperty("isEditable",
                [this]{return m_layer->isEditable();},
                [this](bool i){
                  m_layer->setEditable(i);
                  return i;
                })
      .doc("read+write. Gets/sets whether the layer is editable (unlocked) or not (locked).");

    addProperty("isMovable", [this]{return m_layer->isMovable();})
      .doc("read-only. Returns true if the layer is movable.");

    addProperty("isContinuous", [this]{return m_layer->isContinuous();})
      .doc("read-only. Prefer to link cels when the user copies them.");

    addProperty("flags", [this]{return (int) m_layer->flags();})
      .doc("read-only. Returns all flags OR'd together as an int");

    addProperty("celCount", [this]{
      return m_layer->isImage() ? static_cast<doc::LayerImage*>(m_layer)->getCelsCount() : 0;
    })
      .doc("read-only. Returns the number of cels.");

    addMethod("cel", &LayerScriptObject::cel)
      .doc("retrieves a Cel")
      .docArg("index", "The number of the Cel")
      .docReturns("A Cel object or null if an invalid index is passed");
  }

  ScriptObject* cel(int i){
    auto cel = m_layer->cel(i);
    if (!cel)
      return nullptr;
    auto it = m_cels.find(cel.get());
    if (it == m_cels.end()) {
      it = m_cels.emplace(cel.get(), "CelScriptObject").first;
      it->second->setWrapped(cel.get());
    }
    return it->second.get();
  }

  void* getWrapped() override {return m_layer;}
  void setWrapped(void* layer) override { m_layer = static_cast<doc::Layer*>(layer); }

  doc::Layer* m_layer;
  std::unordered_map<doc::Cel*, inject<ScriptObject>> m_cels;
};

static script::ScriptObject::Regular<LayerScriptObject> layerSO("LayerScriptObject");
