// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "script/script_object.h"
#include "script/value.h"
#include "ui/widget.h"

class WidgetScriptObject : public script::ScriptObject {
protected:
  ui::Widget::Handle m_widget;
  virtual ui::Widget* build() = 0;

  WidgetScriptObject() {
    addProperty("id",
                [this]{
                  auto widget = getWidget();
                  return widget ? widget->id() : "";
                },
                [this](const std::string& id){
                  auto widget = getWidget();
                  if (widget && widget->id().empty()) {
                    widget->setId(id.c_str());
                  }
                  return id;
                });
  }

  ~WidgetScriptObject() {
    auto widget = getWidget();
    if (widget) {
      widget->removeAllChildren();
      if (widget->parent())
        widget->parent()->removeChild(widget);
      delete widget;
    }
  }

  ui::Widget* getWidget() {
    return m_widget.get<ui::Widget>();
  }

public:
  enum class DisplayType {
    Inherit,
    Block,
    Inline
  };

  virtual DisplayType getDisplayType() {return DisplayType::Inherit;}

  template<typename Type>
  Type* getWrapped(){ return static_cast<Type*>(static_cast<ui::Widget*>(getWrapped())); }

  void* getWrapped() override {
    auto handle = m_widget.get<ui::Widget>();
    if (handle)
      return handle;
    auto raw = build();
    if (raw)
      m_widget = raw->handle();
    return raw;
  }

  void setWrapped(void* widget) override {
    auto raw = static_cast<ui::Widget*>(widget);
    if (raw) {
      m_widget = raw->handle();
    } else {
      m_widget.reset();
    }
  }
};
