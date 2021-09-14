// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/signal.h"
#include "ui/custom_label.h"

#include <string>

namespace ui {

  class LinkLabel : public CustomLabel {
  public:
    LinkLabel(const std::string& urlOrText = "");
    LinkLabel(const std::string& url, const std::string& text);

    const std::string& url() const { return m_url; }
    void setUrl(const std::string& url);

    base::Signal0<void> Click;

  protected:
    bool onProcessMessage(Message* msg) override;
    void onPaint(PaintEvent& ev) override;
    virtual void onClick();

    std::string m_url;
  };

} // namespace ui
