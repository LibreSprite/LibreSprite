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

class DialogExtension : public Extension {
public:
  DialogExtension() {
    auto& cls = addClass<void, DialogObject>("Dialog");
    // The dialog is created by app.createDialog() (C++), not `new Dialog()` in
    // JS, but delta requires a non-null constructor. This makes `new Dialog()`
    // also work (and is never used by scripts).
    cls.setConstructor() = []() -> std::shared_ptr<DialogObject> {
      return std::make_shared<DialogObject>();
    };

    cls.addGetter("title") = [](DialogObject& self) -> JSON::Value {
      return self.dialog() ? std::string{self.dialog()->text()} : std::string{};
    };
    cls.addSetter("title") = [](DialogObject& self, JSON::Value& v) {
      if (self.dialog())
        self.dialog()->setText(v.toString());
    };

    cls.addGetter("width") = [](DialogObject& self) -> JSON::Value {
      return self.dialog() ? (double)self.dialog()->size().w : (double)0;
    };
    cls.addGetter("height") = [](DialogObject& self) -> JSON::Value {
      return self.dialog() ? (double)self.dialog()->size().h : (double)0;
    };

    // write only: when false, strip the window decorations (the X button).
    cls.addGetter("canClose") = [](DialogObject&) -> JSON::Value { return true; };
    cls.addSetter("canClose") = [](DialogObject& self, JSON::Value& v) {
      if (self.dialog() && !v.boolean())
        self.dialog()->removeDecorativeWidgets();
    };

    // addLabel(text, id) -> the Label object (same instance as get(id)).
    cls.addMethod("addLabel") = [](DialogObject& self, const std::string& text, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addLabel(text, id));
    };
    // addTextBox(text, id) -> the TextBox object.
    cls.addMethod("addTextBox") = [](DialogObject& self, const std::string& text, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addTextBox(text, id));
    };
    // addButton(text, id) -> the Button object.
    cls.addMethod("addButton") = [](DialogObject& self, const std::string& text, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addButton(text, id));
    };
    // addLinkLabel(text, url, id) -> the LinkLabel object.
    cls.addMethod("addLinkLabel") = [](DialogObject& self, const std::string& text, const std::string& url, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addLinkLabel(text, url, id));
    };
    // addCheckBox(text, id) -> the CheckBox object.
    cls.addMethod("addCheckBox") = [](DialogObject& self, const std::string& text, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addCheckBox(text, id));
    };
    // addEntry(text, id) -> the Entry object (also adds a label with text if
    // non-empty). Matches the old Aseprite API.
    cls.addMethod("addEntry") = [](DialogObject& self, const std::string& text, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addEntry(text, id));
    };
    // addIntEntry(text, id, min, max) -> the IntEntry object (also adds a label
    // with text if non-empty). Matches the old Aseprite API.
    cls.addMethod("addIntEntry") = [](DialogObject& self, const std::string& text, const std::string& id, int min, int max) -> JSON::Value {
      return widgetToNative(self.addIntEntry(text, id, min, max));
    };
    // addDropDown(id) -> the Dropdown object (options are added via
    // Dropdown::addOption).
    cls.addMethod("addDropDown") = [](DialogObject& self, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addDropDown(id));
    };
    // addListBox(id) -> the ListBox object (items are added via ListBox::add()).
    cls.addMethod("addListBox") = [](DialogObject& self, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addListBox(id));
    };
    // addSlider(id, min, max) -> the Slider object (initial value is min).
    cls.addMethod("addSlider") = [](DialogObject& self, const std::string& id, int min, int max) -> JSON::Value {
      return widgetToNative(self.addSlider(id, min, max));
    };
    // addImageView(id) -> the ImageView object.
    cls.addMethod("addImageView") = [](DialogObject& self, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addImageView(id));
    };
    // addPaletteListBox(id) -> the PaletteListBox object.
    cls.addMethod("addPaletteListBox") = [](DialogObject& self, const std::string& id) -> JSON::Value {
      return widgetToNative(self.addPaletteListBox(id));
    };
    // addColor(id, color) -> the Color object (color is a 32-bit RGBA int,
    // the pixelColor.rgba() format).
    cls.addMethod("addColor") = [](DialogObject& self, const std::string& id, double color) -> JSON::Value {
      return widgetToNative(self.addColor(id, (uint32_t)color));
    };
    // addBreak() -> end the current row so the next add*() starts a new one.
    cls.addMethod("addBreak") = [](DialogObject& self) -> JSON::Value {
      self.addBreak();
      return JSON::Value{true};
    };

    // get(id) -> the child widget object, or null.
    cls.addMethod("get") = [](DialogObject& self, const std::string& id) -> JSON::Value {
      return widgetToNative(self.get(id));
    };

    // close() -> hide + remove from the manager (the DialogObject still owns
    // the ui::Dialog and deletes it on destruction).
    cls.addMethod("close") = [](DialogObject& self) -> JSON::Value {
      self.close();
      return JSON::Value{JSON::Special::Undefined};
    };
  }
};

static di::provide<Extension, DialogExtension> dialogExt{"dialog"};
