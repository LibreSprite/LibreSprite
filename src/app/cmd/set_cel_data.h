// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/cel_data.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class SetCelData : public Cmd
                   , public WithCel {
  public:
    SetCelData(Cel* cel, const CelDataRef& newData);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_dataCopy ? m_dataCopy->getMemSize(): 0);
    }

  private:
    void createCopy();

    ObjectId m_oldDataId;
    ObjectId m_oldImageId;
    ObjectId m_newDataId;
    CelDataRef m_dataCopy;

    // Reference used only to keep the copy of the new CelData from
    // the SetCelData() ctor until the SetCelData::onExecute() call.
    // Then the reference is not used anymore.
    CelDataRef m_newData;
  };

} // namespace cmd
} // namespace app
