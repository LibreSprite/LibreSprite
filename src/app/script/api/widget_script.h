// LibreSprite
// Copyright (C) 2023-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// Ownership model (re-mapped from the old Handle/disposable()/build() model
// onto delta's shared_ptr wrappers):
//   - The DialogObject OWNS the ui::Dialog (an owning shared_ptr). Deleting
//     the dialog runs ~ui::Widget, which deletes the whole child subtree
//     (grid -> widgets), so the child widgets need no separate owner.
//   - Child widget objects (LabelObject, ButtonObject, ...) hold a NON-owning
//     pointer to their ui::Widget; the dialog's parent-child tree owns it. A
//     child wrapper that outlives the dialog is stale (documented migration
//     decision) but never double-frees.
//   - ui::Dialog::build() (grid layout + openWindow) is deferred via
//     AppScripting::afterEval so all children are added before the window is
//     laid out and shown.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "delta/Extension.hpp"
#include "ui/widget.h"
#include "ui/label.h"
#include "ui/link_label.h"
#include "ui/textbox.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/listbox.h"
#include "ui/entry.h"
#include "ui/int_entry.h"
#include "ui/slider.h"
#include "ui/image_view.h"
#include "app/ui/color_button.h"
#include "app/ui/dialog.h"
#include "app/ui/palette_listbox.h"
#include "doc/palette.h"
#include "app/script/app_scripting.h"
#include "app/script/api/storage_internal.h"

// Base for dialog-widget wrappers. Holds a non-owning ui::Widget pointer.
class WidgetObject {
public:
  explicit WidgetObject(ui::Widget* widget) : m_widget(widget) {}
  virtual ~WidgetObject() = default;

  ui::Widget* widget() const { return m_widget; }
  template<typename T>
  T* as() const { return dynamic_cast<T*>(m_widget); }

  std::string id() const { return m_widget ? m_widget->id() : ""; }
  void setId(const std::string& id) {
    if (m_widget && m_widget->id().empty())
      m_widget->setId(id.c_str());
  }

protected:
  ui::Widget* m_widget; // non-owning (the dialog owns it)
};

// Add the shared `id` property to a widget's delta class.
template<typename T>
void addWidgetId(Extension::ExportableClass& cls) {
  cls.addGetter("id") = [](T& self) -> JSON::Value { return self.id(); };
  cls.addSetter("id") = [](T& self, JSON::Value& v) { self.setId(v.toString()); };
}

// A text label (ui::Label). Created with empty text; the text is set via the
// `text` property (or by DialogObject::addLabel).
class LabelObject : public WidgetObject {
public:
  LabelObject() : WidgetObject(new ui::Label("")) {}
  ui::Label* label() const { return as<ui::Label>(); }
};

// A read-only multiline text display (ui::TextBox, like a multiline label;
// not editable). Created with empty text and default align; the text and
// align are set via the properties (or by DialogObject::addTextBox).
class TextBoxObject : public WidgetObject {
public:
  TextBoxObject() : WidgetObject(new ui::TextBox("", 0)) {}
  ui::TextBox* textBox() const { return as<ui::TextBox>(); }
};

// A push button (ui::Button). Raises `<id>_click` when clicked.
class ButtonObject : public WidgetObject {
public:
  ButtonObject() : WidgetObject(new ui::Button("")) {
    auto* button = as<ui::Button>();
    std::string fileName = app::AppScripting::getFileName();
    button->Click.connect([this, fileName](ui::Event&) {
      if (!m_widget)
        return;
      JSON::Value event;
      event.push_back(m_widget->id() + "_click");
      app::AppScripting::raiseEvent(fileName, event);
    });
  }
  ui::Button* button() const { return as<ui::Button>(); }
};

// A clickable text label (ui::LinkLabel) that opens its URL in the OS
// handler and raises `<id>_click` when clicked. Created with empty text
// and url; both are set via the properties (or by
// DialogObject::addLinkLabel).
class LinkLabelObject : public WidgetObject {
public:
  LinkLabelObject() : WidgetObject(new ui::LinkLabel("", "")) {
    auto* linkLabel = as<ui::LinkLabel>();
    std::string fileName = app::AppScripting::getFileName();
    linkLabel->Click.connect([this, fileName]() {
      if (!m_widget)
        return;
      JSON::Value event;
      event.push_back(m_widget->id() + "_click");
      app::AppScripting::raiseEvent(fileName, event);
    });
  }
  ui::LinkLabel* linkLabel() const { return as<ui::LinkLabel>(); }
};

// A check box (ui::CheckBox subclass) that persists its checked state to
// storage and raises `<id>_change` on state changes. setCheckedSilent() sets
// the state without raising the event (but still persists it).
class CustomCheckBox : public ui::CheckBox {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomCheckBox(const std::string& fileName, const std::string& domain)
    : ui::CheckBox{""}, m_fileName(fileName), m_domain(domain) {}

  void setCheckedSilent(bool checked) {
    m_canRaiseEvent = false;
    setSelected(checked);
    script::setStorage(JSON::Value{isSelected()}, id(), m_domain);
    m_canRaiseEvent = true;
  }

protected:
  void onSelect() override {
    script::setStorage(JSON::Value{isSelected()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }

  void onDeselect() override {
    script::setStorage(JSON::Value{isSelected()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

// A check box (CustomCheckBox). Raises `<id>_click` when clicked (the checked
// state has already toggled by then).
class CheckBoxObject : public WidgetObject {
public:
  CheckBoxObject() : WidgetObject(new CustomCheckBox(app::AppScripting::getFileName(),
                                                     script::defaultStorageDomain())) {
    auto* checkBox = as<CustomCheckBox>();
    std::string fileName = app::AppScripting::getFileName();
    checkBox->Click.connect([this, fileName](ui::Event&) {
      if (!m_widget)
        return;
      JSON::Value event;
      event.push_back(m_widget->id() + "_click");
      app::AppScripting::raiseEvent(fileName, event);
    });
  }
  CustomCheckBox* checkBox() const { return as<CustomCheckBox>(); }
};

// A text entry (ui::Entry subclass) that persists its value to storage and
// raises `<id>_change` on user edits. setTextSilent() sets the text without
// raising the event (but still persists it).
class CustomEntry : public ui::Entry {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomEntry(std::size_t maxsize, const std::string& fileName, const std::string& domain)
    : ui::Entry{maxsize, nullptr}, m_fileName(fileName), m_domain(domain) {}

  void setTextSilent(const std::string& text) {
    m_canRaiseEvent = false;
    setText(text);
    script::setStorage(JSON::Value{text}, id(), m_domain);
    m_canRaiseEvent = true;
  }

protected:
  void onChange() override {
    script::setStorage(JSON::Value{text()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class EntryObject : public WidgetObject {
public:
  EntryObject() : WidgetObject(new CustomEntry(40, app::AppScripting::getFileName(),
                                               script::defaultStorageDomain())) {}
  CustomEntry* entry() const { return as<CustomEntry>(); }
};

// A dropdown (ui::ComboBox subclass) that persists the selected item's text to
// storage and raises `<id>_change` on selection changes. The *Silent() setters
// set the selection without raising the event (but still persist it).
class CustomComboBox : public ui::ComboBox {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomComboBox(const std::string& fileName, const std::string& domain)
    : m_fileName(fileName), m_domain(domain) {}

  // Shadows ui::ComboBox::getValue() (the item "value" field, empty for items
  // added via addItem(text)): for the script API the value is the selected
  // item's text ("" if none).
  std::string getValue() const {
    int index = getSelectedItemIndex();
    return (index >= 0 ? getItemText(index) : std::string());
  }

  void setValueSilent(const std::string& value) {
    m_canRaiseEvent = false;
    setSelectedItemIndex(findItemIndex(value));
    script::setStorage(JSON::Value{getValue()}, id(), m_domain);
    m_canRaiseEvent = true;
  }

  void setSelectedIndexSilent(int index) {
    m_canRaiseEvent = false;
    setSelectedItemIndex(index);
    script::setStorage(JSON::Value{getValue()}, id(), m_domain);
    m_canRaiseEvent = true;
  }

protected:
  void onChange() override {
    script::setStorage(JSON::Value{getValue()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class ComboBoxObject : public WidgetObject {
public:
  ComboBoxObject() : WidgetObject(new CustomComboBox(app::AppScripting::getFileName(),
                                                     script::defaultStorageDomain())) {}
  CustomComboBox* comboBox() const { return as<CustomComboBox>(); }
};

// A list box (ui::ListBox subclass) that persists the selected item's text to
// storage and raises `<id>_change` on selection changes. selectIndexSilent()
// sets the selection without raising the event (but still persists it).
class CustomListBox : public ui::ListBox {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomListBox(const std::string& fileName, const std::string& domain)
    : m_fileName(fileName), m_domain(domain) {}

  // The selected item's text ("" if none).
  std::string getSelectedText() {
    Widget* child = getSelectedChild();
    return (child ? child->text() : std::string());
  }

  void selectIndexSilent(int index) {
    m_canRaiseEvent = false;
    selectIndex(index);
    script::setStorage(JSON::Value{getSelectedText()}, id(), m_domain);
    m_canRaiseEvent = true;
  }

protected:
  void onChange() override {
    script::setStorage(JSON::Value{getSelectedText()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class ListBoxObject : public WidgetObject {
public:
  ListBoxObject() : WidgetObject(new CustomListBox(app::AppScripting::getFileName(),
                                                   script::defaultStorageDomain())) {}
  CustomListBox* listBox() const { return as<CustomListBox>(); }
};

// An integer entry (ui::IntEntry subclass) that persists its value to storage
// and raises `<id>_change` on user edits. setValueSilent() sets the value
// without raising the event.
class CustomIntEntry : public ui::IntEntry {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomIntEntry(const std::string& fileName, const std::string& domain)
    : ui::IntEntry(0, 100), m_fileName(fileName), m_domain(domain) {}

  void setValueSilent(int value) {
    m_canRaiseEvent = false;
    setValue(value);
    m_canRaiseEvent = true;
  }

protected:
  void onValueChange() override {
    script::setStorage(JSON::Value{(double)getValue()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class IntEntryObject : public WidgetObject {
public:
  IntEntryObject() : WidgetObject(new CustomIntEntry(app::AppScripting::getFileName(),
                                                    script::defaultStorageDomain())) {}
  CustomIntEntry* intEntry() const { return as<CustomIntEntry>(); }
};

// A slider (ui::Slider subclass) that persists its value to storage and
// raises `<id>_change` on user edits. setValueSilent() sets the value
// without raising the event (but still persists it).
class CustomSlider : public ui::Slider {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomSlider(int min, int max, const std::string& fileName, const std::string& domain)
    : ui::Slider(min, max, min), m_fileName(fileName), m_domain(domain) {}

  void setValueSilent(int value) {
    m_canRaiseEvent = false;
    setValue(value);
    script::setStorage(JSON::Value{(double)getValue()}, id(), m_domain);
    m_canRaiseEvent = true;
  }

protected:
  void onChange() override {
    script::setStorage(JSON::Value{(double)getValue()}, id(), m_domain);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class SliderObject : public WidgetObject {
public:
  SliderObject() : WidgetObject(new CustomSlider(0, 100, app::AppScripting::getFileName(),
                                                 script::defaultStorageDomain())) {}
  CustomSlider* slider() const { return as<CustomSlider>(); }
};

// An image view (ui::ImageView). Displays pixel data via putImageData.
class ImageViewObject : public WidgetObject {
public:
  ImageViewObject() : WidgetObject(new ui::ImageView()) {}
  ui::ImageView* imageView() const { return as<ui::ImageView>(); }
};

// A palette listbox (app::PaletteListBox subclass) that persists the selected
// palette name to storage and raises `<id>_change` when the selection changes.
class CustomPaletteListBox : public app::PaletteListBox {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
public:
  CustomPaletteListBox(const std::string& fileName, const std::string& domain)
    : m_fileName(fileName), m_domain(domain) {}

protected:
  void onChange() override {
    script::setStorage(JSON::Value{selectedPaletteName()}, id(), m_domain);
    JSON::Value event;
    event.push_back(id() + "_change");
    app::AppScripting::raiseEvent(m_fileName, event);
  }
};

class PaletteListBoxObject : public WidgetObject {
public:
  PaletteListBoxObject() : WidgetObject(new CustomPaletteListBox(app::AppScripting::getFileName(),
                                                                 script::defaultStorageDomain())) {}
  CustomPaletteListBox* listbox() const { return as<CustomPaletteListBox>(); }
};

// Convert an app::Color to a 32-bit RGBA int (the pixelColor.rgba() format),
// which is what the `color` property and storage persist.
inline uint32_t colorToRgbaInt(const app::Color& color) {
  return doc::rgba((uint8_t)color.getRed(), (uint8_t)color.getGreen(),
                   (uint8_t)color.getBlue(), (uint8_t)color.getAlpha());
}

// A color button (app::ColorButton subclass). The built-in app::ColorButton
// persists nothing and fires its Change signal from both user edits (color
// picked in the popup) and programmatic setColor(), so it is wrapped: this
// subclass persists the color to storage and raises `<id>_change` only on
// user edits (the popup path), and setColorSilent() sets the color without
// raising the event (but still persists it).
class CustomColorButton : public app::ColorButton {
  std::string m_fileName; // full script path (event routing)
  std::string m_domain;   // storage domain (script basename)
  bool m_canRaiseEvent = true;
public:
  CustomColorButton(const app::Color& color, doc::PixelFormat pixelFormat,
                    const std::string& fileName, const std::string& domain)
    : app::ColorButton(color, pixelFormat), m_fileName(fileName), m_domain(domain) {
    // Persist + raise the change event on user edits. The popup route
    // (onClick -> ColorPopup::ColorChange) goes through app::ColorButton's
    // own onWindowColorChange slot (connected there), which runs before this
    // slot; the silent-set route goes through setColorSilent().
    Change.connect([this](const app::Color& color) {
      script::setStorage(JSON::Value{(double)colorToRgbaInt(color)}, id(), m_domain);
      if (m_canRaiseEvent) {
        JSON::Value event;
        event.push_back(id() + "_change");
        app::AppScripting::raiseEvent(m_fileName, event);
      }
    });
  }

  void setColorSilent(const app::Color& color) {
    m_canRaiseEvent = false;
    setColor(color);
    m_canRaiseEvent = true;
  }
};

class ColorButtonObject : public WidgetObject {
public:
  // Created with the RGB mask color and the RGB pixel format. The app::Color
  // stays RgbType, so nothing consults the current palette and the button
  // works standalone in a script dialog (no document/site needed).
  ColorButtonObject()
    : WidgetObject(new CustomColorButton(app::Color::fromMask(), doc::IMAGE_RGB,
                                         app::AppScripting::getFileName(),
                                         script::defaultStorageDomain())) {}
  CustomColorButton* colorButton() const { return as<CustomColorButton>(); }
};

// The dialog window (ui::Dialog). Owns the dialog and its child widgets.
class DialogObject {
public:
  DialogObject() : m_dialog(std::make_shared<ui::Dialog>()) {
    // Defer grid layout + openWindow until after the current eval finishes,
    // so every add*() call has run first. A weak_ptr so this callback never
    // keeps a dialog alive past its DialogObject.
    std::weak_ptr<ui::Dialog> weak = m_dialog;
    app::AppScripting::afterEval([weak]() {
      if (auto dlg = weak.lock())
        dlg->build();
    });
  }

  ~DialogObject() {
    if (m_dialog && m_dialog->isVisible())
      m_dialog->closeWindow(false, true);
  }

  ui::Dialog* dialog() const { return m_dialog.get(); }

  // Add a child widget of the given concrete type. Adds it to the dialog,
  // assigns the id (if provided), and registers it for get(id).
  template<typename T>
  std::shared_ptr<T> addWidget(const std::string& id) {
    if (!m_dialog)
      return nullptr;
    auto obj = std::make_shared<T>();
    m_dialog->add(obj->widget());
    if (!id.empty()) {
      obj->widget()->setId(id.c_str());
      m_widgets[id] = obj;
    }
    return obj;
  }

  std::shared_ptr<LabelObject> addLabel(const std::string& text, const std::string& id) {
    auto obj = addWidget<LabelObject>(id);
    if (obj)
      obj->label()->setText(text);
    return obj;
  }

  std::shared_ptr<TextBoxObject> addTextBox(const std::string& text, const std::string& id) {
    auto obj = addWidget<TextBoxObject>(id);
    if (obj)
      obj->textBox()->setText(text);
    return obj;
  }

  std::shared_ptr<ButtonObject> addButton(const std::string& text, const std::string& id) {
    auto obj = addWidget<ButtonObject>(id);
    if (obj)
      obj->button()->setText(text);
    return obj;
  }

  std::shared_ptr<LinkLabelObject> addLinkLabel(const std::string& text, const std::string& url, const std::string& id) {
    auto obj = addWidget<LinkLabelObject>(id);
    if (obj) {
      auto* linkLabel = obj->linkLabel();
      linkLabel->setText(text);
      linkLabel->setUrl(url);
    }
    return obj;
  }

  std::shared_ptr<CheckBoxObject> addCheckBox(const std::string& text, const std::string& id) {
    auto obj = addWidget<CheckBoxObject>(id);
    if (obj)
      obj->checkBox()->setText(text);
    return obj;
  }

  // addEntry(text, id): if text is non-empty, also add a label (id + "-label")
  // with that text, then add the entry (id). Matches the old Aseprite API.
  std::shared_ptr<EntryObject> addEntry(const std::string& text, const std::string& id) {
    if (!text.empty())
      addLabel(text, id + "-label");
    return addWidget<EntryObject>(id);
  }

  // addIntEntry(text, id, min, max): add a label (id + "-label") with text, then
  // an int entry (id) with the given min/max. Matches the old Aseprite API.
  std::shared_ptr<IntEntryObject> addIntEntry(const std::string& text, const std::string& id, int min, int max) {
    if (!text.empty())
      addLabel(text, id + "-label");
    auto obj = addWidget<IntEntryObject>(id);
    if (obj) {
      obj->intEntry()->setMin(min);
      obj->intEntry()->setMax(max);
    }
    return obj;
  }

  // addSlider(id, min, max): add a slider with the given range; the initial
  // value is min (the range can be changed later via the min/max properties).
  std::shared_ptr<SliderObject> addSlider(const std::string& id, int min, int max) {
    auto obj = addWidget<SliderObject>(id);
    if (obj)
      obj->slider()->setRange(min, max);
    return obj;
  }

  // addBreak(): end the current row so the next add*() starts a new one.
  void addBreak() {
    if (m_dialog)
      m_dialog->addBreak();
  }

  // addDropDown(id): add an empty dropdown (options are added via
  // Dropdown::addOption).
  std::shared_ptr<ComboBoxObject> addDropDown(const std::string& id) {
    return addWidget<ComboBoxObject>(id);
  }

  // addListBox(id): add an empty list box (items are added via
  // ListBox::add()).
  std::shared_ptr<ListBoxObject> addListBox(const std::string& id) {
    return addWidget<ListBoxObject>(id);
  }

  std::shared_ptr<ImageViewObject> addImageView(const std::string& id) {
    return addWidget<ImageViewObject>(id);
  }

  std::shared_ptr<PaletteListBoxObject> addPaletteListBox(const std::string& id) {
    return addWidget<PaletteListBoxObject>(id);
  }

  // addColor(id, color): add a color picker (a swatch that opens the color
  // popup on click). color is a 32-bit RGBA int (pixelColor.rgba() format).
  std::shared_ptr<ColorButtonObject> addColor(const std::string& id, uint32_t color) {
    auto obj = addWidget<ColorButtonObject>(id);
    if (obj)
      obj->colorButton()->setColorSilent(
        app::Color::fromRgb(doc::rgba_getr(color), doc::rgba_getg(color),
                            doc::rgba_getb(color), doc::rgba_geta(color)));
    return obj;
  }

  std::shared_ptr<WidgetObject> get(const std::string& id) {
    auto it = m_widgets.find(id);
    return it != m_widgets.end() ? it->second : nullptr;
  }

  void close() {
    if (m_dialog)
      m_dialog->closeWindow(false, true);
  }

private:
  std::shared_ptr<ui::Dialog> m_dialog; // owning
  std::unordered_map<std::string, std::shared_ptr<WidgetObject>> m_widgets;
};

// Convert a WidgetObject to a JSON::Value (via JSON::makeNative). Checks the
// concrete type and returns the appropriate native reference (makeNative needs
// the concrete registered type, not the base WidgetObject).
inline JSON::Value widgetToNative(std::shared_ptr<WidgetObject> base) {
  if (!base)
    return JSON::Value{JSON::Special::Null};
  if (auto* label = dynamic_cast<LabelObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<LabelObject>(base, label));
  if (auto* textBox = dynamic_cast<TextBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<TextBoxObject>(base, textBox));
  if (auto* button = dynamic_cast<ButtonObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ButtonObject>(base, button));
  if (auto* linkLabel = dynamic_cast<LinkLabelObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<LinkLabelObject>(base, linkLabel));
  if (auto* checkBox = dynamic_cast<CheckBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<CheckBoxObject>(base, checkBox));
  if (auto* entry = dynamic_cast<EntryObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<EntryObject>(base, entry));
  if (auto* comboBox = dynamic_cast<ComboBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ComboBoxObject>(base, comboBox));
  if (auto* listBox = dynamic_cast<ListBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ListBoxObject>(base, listBox));
  if (auto* intEntry = dynamic_cast<IntEntryObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<IntEntryObject>(base, intEntry));
  if (auto* slider = dynamic_cast<SliderObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<SliderObject>(base, slider));
  if (auto* imageView = dynamic_cast<ImageViewObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ImageViewObject>(base, imageView));
  if (auto* paletteListBox = dynamic_cast<PaletteListBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<PaletteListBoxObject>(base, paletteListBox));
  if (auto* colorButton = dynamic_cast<ColorButtonObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ColorButtonObject>(base, colorButton));
  return JSON::Value{JSON::Special::Null};
}
