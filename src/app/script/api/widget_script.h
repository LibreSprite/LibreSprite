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
  WidgetScriptObject() {
    addProperty("id",
                [this]{
                  auto widget = handle<ui::Widget>();
                  return widget ? widget->id() : "";
                },
                [this](const std::string& id){
                  auto widget = handle<ui::Widget>();
                  if (widget && widget->id().empty()) {
                    widget->setId(id.c_str());
                  }
                  return id;
                });
  }

  ~WidgetScriptObject() {
    auto widget = handle<ui::Widget>();
    if (widget) {
      widget->removeAllChildren();
      if (widget->parent())
        widget->parent()->removeChild(widget);
      delete widget;
    }
  }

public:
  enum class DisplayType {
    Inherit,
    Block,
    Inline
  };

  bool disposable() override {
    auto widget = handle<ui::Widget>();
    if (!widget)
      return true;
    return !widget->isVisible();
  }

  virtual DisplayType getDisplayType() {return DisplayType::Inherit;}

  template <typename Type = ui::Widget>
  Type* getWidget() {
    return handle<ui::Widget, Type>();
  }
};
