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
                    });

        addProperty("isImage", [this]{return m_layer->isImage();});

        addProperty("isBackground", [this]{return m_layer->isBackground();});

        addProperty("isTransparent", [this]{return m_layer->isTransparent();});

        addProperty("isVisible",
                    [this]{return m_layer->isVisible();},
                    [this](bool i){
                        m_layer->setVisible(i);
                        return i;
                    });

        addProperty("isEditable",
                    [this]{return m_layer->isEditable();},
                    [this](bool i){
                        m_layer->setEditable(i);
                        return i;
                    });

        addProperty("isMovable", [this]{return m_layer->isMovable();});
        addProperty("isContinuous", [this]{return m_layer->isContinuous();});
        addProperty("flags", [this]{return (int) m_layer->flags();});
        addProperty("celCount", [this]{
            return m_layer->isImage() ? static_cast<doc::LayerImage*>(m_layer)->getCelsCount() : 0;
        });

        addMethod("cel", &LayerScriptObject::cel);
    }

    ScriptObject* cel(int i){
        auto cel = m_layer->cel(i);
        if (!cel)
            return nullptr;
        auto it = m_cels.find(cel);
        if (it == m_cels.end()) {
            it = m_cels.emplace(cel, "CelScriptObject").first;
            it->second->setWrapped(cel);
        }
        return it->second.get();
    }

    void* getWrapped() override {return m_layer;}
    void setWrapped(void* layer) override { m_layer = static_cast<doc::Layer*>(layer); }

    doc::Layer* m_layer;
    std::unordered_map<doc::Cel*, inject<ScriptObject>> m_cels;
};

static script::ScriptObject::Regular<LayerScriptObject> layerSO("LayerScriptObject");
