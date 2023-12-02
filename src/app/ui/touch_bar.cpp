#include "touch_bar.h"
#include "app/pref/preferences.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "ui/base.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/event.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/widget_type.h"
#include <iostream>
#include <map>
#include <memory>

namespace app {

static std::vector<TouchBar*> touchbars;
static std::map<std::string, const Key*> touches;

class Touch {
public:
  TouchBar& touchbar;
  std::shared_ptr<ui::Button> button;

  Touch(TouchBar& touchbar, const std::string& label) : touchbar{touchbar} {
    button = std::make_shared<ui::Button>(label);
    touchbar.addChild(button.get());
    button->Click.connect([&](auto&){activate();});
  }

  ~Touch() {
    touchbar.removeChild(button.get());
  }

  void activate() {
    const Key* key{};
    for (auto candidate : *KeyboardShortcuts::instance()) {
      if (candidate->label() == button->text()) {
        key = candidate;
        break;
      }
    }
    if (!key)
      return;

    switch (key->type()) {
    case KeyType::Quicktool:
    case KeyType::Tool: {
      ToolBar::instance()->selectTool(key->tool());
      break;
    }
    case KeyType::Command: {
      UIContext::instance()->executeCommand(key->command(), key->params());
      break;
    }
    }
  }
};

std::shared_ptr<TouchBar> TouchBar::create(int align) {
  return std::shared_ptr<TouchBar>{new TouchBar(align)};
}

TouchBar::TouchBar(int align) : ui::Box{align} {
  touchbars.push_back(this);
  setBorder({ui::guiscale(), 0, ui::guiscale(), 0});
  auto theme = static_cast<skin::SkinTheme*>(this->theme());
  setBgColor(theme->colors.tabActiveFace());
  for (auto& touch : touches)
    m_touches[touch.first] = std::make_shared<Touch>(*this, touch.first);
  internalOrganize();
}

TouchBar::~TouchBar()
{
  auto it = std::find(touchbars.begin(), touchbars.end(), this);
  if (it != touchbars.end())
    touchbars.erase(it);
}

void TouchBar::removeTouch(const std::string &label)
{
  auto it = touches.find(label);
  if (it == touches.end())
    return;
  touches.erase(it);
  for (auto touchbar : touchbars) {
    touchbar->internalRemoveTouch(label);
  }
}

void TouchBar::organize()
{
  for (auto touchbar : touchbars)
    touchbar->internalOrganize();
}

void TouchBar::internalRemoveTouch(const std::string &label) {
    auto it = m_touches.find(label);
    if (it == m_touches.end()) {
        return;
    }
    removeChild(it->second->button.get());
    m_touches.erase(it);
}

void TouchBar::internalAddTouch(const std::string &label)
{
  m_touches[label] = std::make_shared<Touch>(*this, label);
}

void TouchBar::internalOrganize()
{
  int i{};
  int w{};
  int h{};

  for (auto& entry : touches) {
    auto& button = *m_touches[entry.first]->button;
    w = std::max(w, button.textWidth() * ui::guiscale() + button.border().width() * 2);
    if (!h)
      h = button.textHeight() * ui::guiscale() + button.border().height() * 2;
    removeChild(&button);
  }

  for (auto& entry : touches) {
    auto& button = *m_touches[entry.first]->button;
    addChild(&button);
    button.setBounds({
      0, i++ * h,
      w, h
    });
  }

  layout();
}

void TouchBar::addTouch(const Key& key) {
  auto& label = key.label();
  if (label.empty())
    return;
  if (auto it = touches.find(label); it != touches.end()) {
    touches.erase(it);
    for (auto touchbar : touchbars)
      touchbar->internalRemoveTouch(label);
  }
  touches[label] = &key;
  for (auto touchbar : touchbars)
    touchbar->internalAddTouch(label);
}

}
