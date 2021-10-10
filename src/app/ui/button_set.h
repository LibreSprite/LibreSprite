// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/skin/skin_part.h"
#include "base/signal.h"
#include "ui/grid.h"

#include <string>

namespace app {

  class ButtonSet : public ui::Grid {
  public:
    class Item : public ui::Widget {
    public:
      void setIcon(const skin::SkinPartPtr& icon, bool mono = false);
      skin::SkinPartPtr icon() const { return m_icon; }
      ButtonSet* buttonSet();

    protected:
      Item();
      void onPaint(ui::PaintEvent& ev) override;
      bool onProcessMessage(ui::Message* msg) override;
      void onSizeHint(ui::SizeHintEvent& ev) override;
      virtual void onClick();
      virtual void onRightClick() {}
    private:
      skin::SkinPartPtr m_icon{nullptr};
      bool m_mono;
    };

    std::shared_ptr<Item> addItem(const std::string& text, int hspan = 1, int vspan = 1);
    std::shared_ptr<Item> addItem(const skin::SkinPartPtr& icon, int hspan = 1, int vspan = 1);
    std::shared_ptr<Item> addItem(std::shared_ptr<Item> item, int hspan = 1, int vspan = 1);
    std::shared_ptr<Item> getItem(int index);

    int selectedItem() const;
    void setSelectedItem(int index, bool focusItem = true);
    void setSelectedItem(std::shared_ptr<Item> item, bool focusItem = true);
    void deselectItems();

    void setOfferCapture(bool state);
    void setTriggerOnMouseUp(bool state);
    void setMultipleSelection(bool state);

    base::Signal1<void, Item*> ItemChange;
    base::Signal1<void, Item*> RightClick;

  protected:
    ButtonSet();

    virtual void onItemChange(std::shared_ptr<Item> item);
    virtual void onRightClick(std::shared_ptr<Item> item);

  private:
    std::shared_ptr<Item> findSelectedItem() const;

    bool m_offerCapture = true;
    bool m_triggerOnMouseUp = false;
    bool m_multipleSelection = false;
  };

} // namespace app
