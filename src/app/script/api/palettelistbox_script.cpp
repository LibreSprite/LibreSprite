// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/widget_script.h"
#include "doc/palette.h"

#include <memory>
#include <string>

class PaletteListBoxExtension : public Extension {
public:
  PaletteListBoxExtension() {
    auto& cls = addClass<void, PaletteListBoxObject>("PaletteListBox");
    // The listbox is created by DialogObject::addPaletteListBox() (C++), not
    // `new PaletteListBox()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<PaletteListBoxObject> {
      return std::make_shared<PaletteListBoxObject>();
    };

    addWidgetId<PaletteListBoxObject>(cls);

    // selected: the name of the currently selected palette ("" if none).
    cls.addGetter("selected") = [](PaletteListBoxObject& self) -> JSON::Value {
      auto* lb = self.listbox();
      return lb ? std::string{lb->selectedPaletteName()} : std::string{};
    };

    // addPalette(name) -> create a 1-color doc::Palette, add it to the listbox
    // under `name`, and return it as a `Palette` JS object.
    cls.addMethod("addPalette") = [](PaletteListBoxObject& self, const std::string& name) -> JSON::Value {
      auto* lb = self.listbox();
      if (!lb)
        return JSON::Value{JSON::Special::Null};
      auto pal = doc::Palette::create(1);
      lb->addPalette(pal, name);
      return JSON::makeNative(pal);
    };
  }
};

static di::provide<Extension, PaletteListBoxExtension> paletteListBoxExt{"palettelistbox"};
