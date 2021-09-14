// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/brush_slot.h"
#include "base/signal.h"
#include "doc/brushes.h"

#include <vector>

namespace app {

  class AppBrushes {
  public:
    // Number of slot (a range from 1 to AppBrushes::size() inclusive)
    typedef int slot_id;
    typedef std::vector<BrushSlot> BrushSlots;

    AppBrushes();
    ~AppBrushes();

    // Adds a new brush and returns the slot number where the brush
    // is now available.
    slot_id addBrushSlot(const BrushSlot& brush);
    void removeBrushSlot(slot_id slot);
    void removeAllBrushSlots();
    bool hasBrushSlot(slot_id slot) const;
    const doc::Brushes& getStandardBrushes() { return m_standard; }
    BrushSlot getBrushSlot(slot_id slot) const;
    void setBrushSlot(slot_id slot, const BrushSlot& brush);
    const BrushSlots& getBrushSlots() const { return m_slots; }

    void lockBrushSlot(slot_id slot);
    void unlockBrushSlot(slot_id slot);
    bool isBrushSlotLocked(slot_id slot) const;

    base::Signal0<void> ItemsChange;

  private:
    void load(const std::string& filename);
    void save(const std::string& filename) const;
    static std::string userBrushesFilename();

    doc::Brushes m_standard;
    BrushSlots m_slots;
  };

} // namespace app
