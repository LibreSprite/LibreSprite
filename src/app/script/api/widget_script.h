// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "script/script_object.h"
#include "ui/widget.h"

class WidgetScriptObject : public script::ScriptObject {
public:

  void* getWrapped() override {return m_widget;}
  void setWrapped(void* widget) override {m_widget = static_cast<ui::Widget*>(widget);}

  ui::Widget* m_widget;
};
