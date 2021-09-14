// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"

#include <sstream>

namespace doc {
  class Layer;
}

namespace app {
namespace cmd {
  using namespace doc;

  class AddLayer : public Cmd {
  public:
    AddLayer(Layer* folder, Layer* newLayer, Layer* afterThis);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_size;
    }

  private:
    void addLayer(Layer* folder, Layer* newLayer, Layer* afterThis);
    void removeLayer(Layer* folder, Layer* layer);

    WithLayer m_folder;
    WithLayer m_newLayer;
    WithLayer m_afterThis;
    size_t m_size;
    std::stringstream m_stream;
  };

} // namespace cmd
} // namespace app
