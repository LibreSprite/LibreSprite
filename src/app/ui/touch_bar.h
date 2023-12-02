#pragma once

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/ui/keyboard_shortcuts.h"
#include "ui/box.h"
#include <memory>
#include <unordered_map>

namespace app {
  class Touch;
  class TouchBar : public ui::Box {
    std::unordered_map<std::string, std::shared_ptr<Touch>> m_touches;

  public:
    static void removeTouch(const std::string &label);
    static void addTouch(const Key& key);
    static void organize();
    static std::shared_ptr<app::TouchBar> create(int align);
    ~TouchBar();

  private:
    TouchBar(int align);
    void internalRemoveTouch(const std::string &label);
    void internalAddTouch(const std::string &label);
    void internalOrganize();
  };
}
