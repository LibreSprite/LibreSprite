// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"

#include <memory>

namespace doc {
  class Mask;
}

namespace app {
namespace cmd {
  using namespace doc;

  class ReselectMask : public Cmd
                     , public WithDocument {
  public:
    ReselectMask(Document* doc);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override;

  private:
    std::unique_ptr<Mask> m_oldMask;
  };

} // namespace cmd
} // namespace app
