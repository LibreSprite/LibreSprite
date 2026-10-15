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
#include "ui/listitem.h"

#include <memory>
#include <string>

class ListBoxExtension : public Extension {
public:
  ListBoxExtension() {
    auto& cls = addClass<void, ListBoxObject>("ListBox");
    // The list box is created by DialogObject::addListBox() (C++), not
    // `new ListBox()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<ListBoxObject> {
      return std::make_shared<ListBoxObject>();
    };

    addWidgetId<ListBoxObject>(cls);

    // add(text) -> append a list item and return its index.
    cls.addMethod("add") = [](ListBoxObject& self, const std::string& text) -> JSON::Value {
      auto* lb = self.listBox();
      if (!lb)
        return JSON::Value{JSON::Special::Null};
      const int index = (int)lb->getItemsCount();
      lb->insertChild(index, new ui::ListItem(text));
      return (double)index;
    };

    // remove(index) -> remove the item at the given index.
    cls.addMethod("remove") = [](ListBoxObject& self, double index) -> JSON::Value {
      auto* lb = self.listBox();
      if (lb) {
        const auto& children = lb->children();
        const int i = (int)index;
        if (i >= 0 && i < (int)children.size())
          lb->removeChild(children[i]);
      }
      return JSON::Value{JSON::Special::Undefined};
    };

    // count: the number of items.
    cls.addGetter("count") = [](ListBoxObject& self) -> JSON::Value {
      return self.listBox() ? (double)self.listBox()->getItemsCount() : (double)0;
    };

    // selectedIndex: the selected item's index (-1 if none). Setting it
    // persists to storage but does not raise the change event
    // (selectIndexSilent); user clicks do.
    cls.addGetter("selectedIndex") = [](ListBoxObject& self) -> JSON::Value {
      return self.listBox() ? (double)self.listBox()->getSelectedIndex() : (double)-1;
    };
    cls.addSetter("selectedIndex") = [](ListBoxObject& self, JSON::Value& v) {
      if (self.listBox())
        self.listBox()->selectIndexSilent((int)v.number());
    };

    // selected: the selected item's text ("" if none).
    cls.addGetter("selected") = [](ListBoxObject& self) -> JSON::Value {
      return self.listBox() ? std::string{self.listBox()->getSelectedText()} : std::string{};
    };
  }
};

static di::provide<Extension, ListBoxExtension> listBoxExt{"listbox"};
