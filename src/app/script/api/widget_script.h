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
  ui::Widget* m_widget = nullptr;
  virtual ui::Widget* build() = 0;

  WidgetScriptObject() {
      addProperty("id",
                  [this]{return getWrapped<ui::Widget>()->id();},
                  [this](const std::string& id){
                      auto widget = getWrapped<ui::Widget>();
                      if (widget->id().empty()) {
                          widget->setId(id.c_str());
                      }
                      return widget->id();
                  });
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
  void* getWrapped() override {return m_widget ?: m_widget = build();}
  void setWrapped(void* widget) override {m_widget = static_cast<ui::Widget*>(widget);}
};
