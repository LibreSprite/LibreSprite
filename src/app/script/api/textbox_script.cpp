// LibreSprite
// Copyright (C) 2023-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/widget_script.h"

#include <memory>
#include <string>

class TextBoxExtension : public Extension {
public:
  TextBoxExtension() {
    auto& cls = addClass<void, TextBoxObject>("TextBox");
    // The text box is created by DialogObject::addTextBox() (C++), not
    // `new TextBox()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<TextBoxObject> {
      return std::make_shared<TextBoxObject>();
    };

    addWidgetId<TextBoxObject>(cls);

    cls.addGetter("text") = [](TextBoxObject& self) -> JSON::Value {
      auto* textBox = self.textBox();
      return textBox ? std::string{textBox->text()} : std::string{};
    };
    cls.addSetter("text") = [](TextBoxObject& self, JSON::Value& v) {
      if (auto* textBox = self.textBox())
        textBox->setText(v.toString());
    };

    // align: the widget's alignment flags (ui flags in ui/base.h). OR the
    // WORDWRAP flag (0x02000000) to enable text wrapping; LEFT/CENTER/RIGHT
    // (0x00040000/0x00080000/0x00100000) and TOP/MIDDLE/BOTTOM
    // (0x00200000/0x00400000/0x00800000) select the position within the
    // widget.
    cls.addGetter("align") = [](TextBoxObject& self) -> JSON::Value {
      return self.textBox() ? (double)self.textBox()->align() : (double)0;
    };
    cls.addSetter("align") = [](TextBoxObject& self, JSON::Value& v) {
      if (self.textBox())
        self.textBox()->setAlign((int)v.number());
    };
  }
};

static di::provide<Extension, TextBoxExtension> textBoxExt{"textbox"};
