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

#include <memory>
#include <string>

class SliderExtension : public Extension {
public:
  SliderExtension() {
    auto& cls = addClass<void, SliderObject>("Slider");
    // The slider is created by DialogObject::addSlider() (C++), not
    // `new Slider()` in JS, but delta requires a non-null constructor.
    cls.setConstructor() = []() -> std::shared_ptr<SliderObject> {
      return std::make_shared<SliderObject>();
    };

    addWidgetId<SliderObject>(cls);

    // min: the minimum value.
    cls.addGetter("min") = [](SliderObject& self) -> JSON::Value {
      return self.slider() ? (double)self.slider()->getMinValue() : (double)0;
    };
    cls.addSetter("min") = [](SliderObject& self, JSON::Value& v) {
      if (self.slider())
        self.slider()->setRange((int)v.number(), self.slider()->getMaxValue());
    };

    // max: the maximum value.
    cls.addGetter("max") = [](SliderObject& self) -> JSON::Value {
      return self.slider() ? (double)self.slider()->getMaxValue() : (double)100;
    };
    cls.addSetter("max") = [](SliderObject& self, JSON::Value& v) {
      if (self.slider())
        self.slider()->setRange(self.slider()->getMinValue(), (int)v.number());
    };

    // value: the integer value. Setting it persists to storage but does not
    // raise the change event (setValueSilent); user edits do.
    cls.addGetter("value") = [](SliderObject& self) -> JSON::Value {
      return self.slider() ? (double)self.slider()->getValue() : (double)0;
    };
    cls.addSetter("value") = [](SliderObject& self, JSON::Value& v) {
      if (self.slider())
        self.slider()->setValueSilent((int)v.number());
    };
  }
};

static di::provide<Extension, SliderExtension> sliderExt{"slider"};
