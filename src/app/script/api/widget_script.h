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
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/int_entry.h"
#include "ui/image_view.h"
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

// A text entry (ui::Entry subclass) that persists its value to storage and
// raises `<id>_change` on user edits. setTextSilent() sets the text without
// raising the event (but still persists it).
class CustomEntry : public ui::Entry {
  std::string m_fileName;
  bool m_canRaiseEvent = true;
public:
  CustomEntry(std::size_t maxsize, const std::string& fileName)
    : ui::Entry{maxsize, nullptr}, m_fileName(fileName) {}

  void setTextSilent(const std::string& text) {
    m_canRaiseEvent = false;
    setText(text);
    script::setStorage(JSON::Value{text}, id(), m_fileName);
    m_canRaiseEvent = true;
  }

protected:
  void onChange() override {
    script::setStorage(JSON::Value{text()}, id(), m_fileName);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class EntryObject : public WidgetObject {
public:
  EntryObject() : WidgetObject(new CustomEntry(40, app::AppScripting::getFileName())) {}
  CustomEntry* entry() const { return as<CustomEntry>(); }
};

// An integer entry (ui::IntEntry subclass) that persists its value to storage
// and raises `<id>_change` on user edits. setValueSilent() sets the value
// without raising the event.
class CustomIntEntry : public ui::IntEntry {
  std::string m_fileName;
  bool m_canRaiseEvent = true;
public:
  CustomIntEntry(const std::string& fileName) : ui::IntEntry(0, 100), m_fileName(fileName) {}

  void setValueSilent(int value) {
    m_canRaiseEvent = false;
    setValue(value);
    m_canRaiseEvent = true;
  }

protected:
  void onValueChange() override {
    script::setStorage(JSON::Value{(double)getValue()}, id(), m_fileName);
    if (m_canRaiseEvent) {
      JSON::Value event;
      event.push_back(id() + "_change");
      app::AppScripting::raiseEvent(m_fileName, event);
    }
  }
};

class IntEntryObject : public WidgetObject {
public:
  IntEntryObject() : WidgetObject(new CustomIntEntry(app::AppScripting::getFileName())) {}
  CustomIntEntry* intEntry() const { return as<CustomIntEntry>(); }
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
  std::string m_fileName;
public:
  CustomPaletteListBox(const std::string& fileName) : m_fileName(fileName) {}

protected:
  void onChange() override {
    script::setStorage(JSON::Value{selectedPaletteName()}, id(), m_fileName);
    JSON::Value event;
    event.push_back(id() + "_change");
    app::AppScripting::raiseEvent(m_fileName, event);
  }
};

class PaletteListBoxObject : public WidgetObject {
public:
  PaletteListBoxObject() : WidgetObject(new CustomPaletteListBox(app::AppScripting::getFileName())) {}
  CustomPaletteListBox* listbox() const { return as<CustomPaletteListBox>(); }
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

  std::shared_ptr<ButtonObject> addButton(const std::string& text, const std::string& id) {
    auto obj = addWidget<ButtonObject>(id);
    if (obj)
      obj->button()->setText(text);
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

  // addBreak(): end the current row so the next add*() starts a new one.
  void addBreak() {
    if (m_dialog)
      m_dialog->addBreak();
  }

  std::shared_ptr<ImageViewObject> addImageView(const std::string& id) {
    return addWidget<ImageViewObject>(id);
  }

  std::shared_ptr<PaletteListBoxObject> addPaletteListBox(const std::string& id) {
    return addWidget<PaletteListBoxObject>(id);
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
  if (auto* button = dynamic_cast<ButtonObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ButtonObject>(base, button));
  if (auto* entry = dynamic_cast<EntryObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<EntryObject>(base, entry));
  if (auto* intEntry = dynamic_cast<IntEntryObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<IntEntryObject>(base, intEntry));
  if (auto* imageView = dynamic_cast<ImageViewObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<ImageViewObject>(base, imageView));
  if (auto* paletteListBox = dynamic_cast<PaletteListBoxObject*>(base.get()))
    return JSON::makeNative(std::shared_ptr<PaletteListBoxObject>(base, paletteListBox));
  return JSON::Value{JSON::Special::Null};
}
