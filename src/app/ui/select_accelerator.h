// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/keyboard_shortcuts.h"
#include "ui/accelerator.h"
#include "ui/tooltips.h"

#include "select_accelerator.xml.h"

namespace app {

  class SelectAccelerator : public app::gen::SelectAccelerator {
  public:
    SelectAccelerator(const ui::Accelerator& accelerator, KeyContext keyContext);

    bool isModified() const { return m_modified; }
    const ui::Accelerator& accel() const { return m_accel; }

  private:
    void onModifierChange(ui::KeyModifiers modifier, ui::CheckBox* checkbox);
    void onAccelChange(const ui::Accelerator* accel);
    void onClear();
    void onOK();
    void onCancel();
    void updateModifiers();
    void updateAssignedTo();

    class KeyField;

    ui::TooltipManager m_tooltipManager;
    KeyField* m_keyField;
    KeyContext m_keyContext;
    ui::Accelerator m_origAccel;
    ui::Accelerator m_accel;
    bool m_modified;
  };

} // namespace app
