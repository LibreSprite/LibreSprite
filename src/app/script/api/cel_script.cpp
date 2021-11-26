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

class CelScriptObject : public script::ScriptObject {
public:
  CelScriptObject() {
    addProperty("x",
                [this]{return m_cel->x();},
                [this](int x){m_cel->setPosition(x, m_cel->y()); return x;});
    addProperty("y",
                [this]{return m_cel->y();},
                [this](int y){m_cel->setPosition(m_cel->x(), y); return y;});
    addProperty("image", [this]{return m_image.get();});
    addProperty("frame", [this]{return m_cel->frame();});
    addMethod("setPosition", &CelScriptObject::setPosition);
  }

  void setPosition(int x, int y){
    m_cel->setPosition(x, y);
  }

  void* getWrapped() override {return m_cel;}
  void setWrapped(void* cel) override {
    m_cel = static_cast<doc::Cel*>(cel);
    auto image = m_cel->image();
    if (!image) {
      auto sprite = m_cel->sprite();
      std::shared_ptr<doc::Image> imgref(doc::Image::create(sprite->pixelFormat(), sprite->width(), sprite->height()));
      m_cel->data()->setImage(imgref);
    }
    m_image->setWrapped(image);
  }

  inject<ScriptObject> m_image{"ImageScriptObject"};
  doc::Cel* m_cel;
};

static script::ScriptObject::Regular<CelScriptObject> celSO("CelScriptObject");
