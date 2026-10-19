// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_flags.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/timeline.h"
#include "app/ui/user_data_popup.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/log.h"
#include "base/scoped_value.h"
#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/user_data.h"
#include "ui/ui.h"

#include <stdexcept>

#include "layer_properties.xml.h"

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();
  Command* clone() const override { return new LayerPropertiesCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  // Property params: with any of them the changes are applied in one
  // transaction without the window.
  bool m_hasParams;
  bool m_hasName;
  std::string m_name;
  bool m_hasOpacity;
  int m_opacity;
  bool m_hasBlendMode;
  doc::BlendMode m_blendMode;
  bool m_hasVisible;
  bool m_visible;
  bool m_hasUserData;
  std::string m_userData;
};

// Converts the blend-mode param to a BlendMode (the UI combo names).
static doc::BlendMode blend_mode_from_string(const std::string& s)
{
  if (s == "normal") return doc::BlendMode::NORMAL;
  if (s == "multiply") return doc::BlendMode::MULTIPLY;
  if (s == "screen") return doc::BlendMode::SCREEN;
  if (s == "overlay") return doc::BlendMode::OVERLAY;
  if (s == "darken") return doc::BlendMode::DARKEN;
  if (s == "lighten") return doc::BlendMode::LIGHTEN;
  if (s == "color-dodge") return doc::BlendMode::COLOR_DODGE;
  if (s == "color-burn") return doc::BlendMode::COLOR_BURN;
  if (s == "hard-light") return doc::BlendMode::HARD_LIGHT;
  if (s == "soft-light") return doc::BlendMode::SOFT_LIGHT;
  if (s == "difference") return doc::BlendMode::DIFFERENCE;
  if (s == "exclusion") return doc::BlendMode::EXCLUSION;
  if (s == "hue") return doc::BlendMode::HSL_HUE;
  if (s == "saturation") return doc::BlendMode::HSL_SATURATION;
  if (s == "color") return doc::BlendMode::HSL_COLOR;
  if (s == "luminosity") return doc::BlendMode::HSL_LUMINOSITY;

  throw std::runtime_error(
    "LayerProperties: invalid blend-mode '" + s + "' (expected one of: "
    "normal, multiply, screen, overlay, darken, lighten, color-dodge, "
    "color-burn, hard-light, soft-light, difference, exclusion, hue, "
    "saturation, color, luminosity)");
}

class LayerPropertiesWindow;
static LayerPropertiesWindow* g_window = nullptr;

class LayerPropertiesWindow : public app::gen::LayerProperties
                            , public doc::ContextObserver
                            , public doc::DocumentObserver {
public:
  LayerPropertiesWindow()
    : m_timer(250, this)
    , m_layer(nullptr)
    , m_selfUpdate(false) {
    name()->setMinSize(gfx::Size(128, 0));
    name()->setExpansive(true);

    mode()->addItem("Normal");
    mode()->addItem("Multiply");
    mode()->addItem("Screen");
    mode()->addItem("Overlay");
    mode()->addItem("Darken");
    mode()->addItem("Lighten");
    mode()->addItem("Color Dodge");
    mode()->addItem("Color Burn");
    mode()->addItem("Hard Light");
    mode()->addItem("Soft Light");
    mode()->addItem("Difference");
    mode()->addItem("Exclusion");
    mode()->addItem("Hue");
    mode()->addItem("Saturation");
    mode()->addItem("Color");
    mode()->addItem("Luminosity");

    name()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    mode()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    opacity()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    m_timer.Tick.connect(base::Bind<void>(&LayerPropertiesWindow::onCommitChange, this));
    userData()->Click.connect(base::Bind<void>(&LayerPropertiesWindow::onPopupUserData, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "LayerProperties");

    UIContext::instance()->addObserver(this);
  }

  ~LayerPropertiesWindow() {
    UIContext::instance()->removeObserver(this);
  }

  void setLayer(LayerImage* layer) {
    if (m_layer) {
      document()->removeObserver(this);
      m_layer = nullptr;
    }

    m_timer.stop();
    m_layer = const_cast<LayerImage*>(layer);

    if (m_layer)
      document()->addObserver(this);

    updateFromLayer();
  }

private:

  app::Document* document() {
    ASSERT(m_layer);
    if (m_layer)
      return static_cast<app::Document*>(m_layer->sprite()->document());
    else
      return nullptr;
  }

  std::string nameValue() const {
    return name()->text();
  }

  BlendMode blendModeValue() const {
    return (BlendMode)mode()->getSelectedItemIndex();
  }

  int opacityValue() const {
    return opacity()->getValue();
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case kKeyDownMessage:
        if (name()->hasFocus() ||
            opacity()->hasFocus() ||
            mode()->hasFocus()) {
          KeyScancode scancode = static_cast<KeyMessage*>(msg)->scancode();
          if (scancode == kKeyEnter || scancode == kKeyEsc) {
            onCommitChange();
            closeWindow(this);
            return true;
          }
        }
        break;

      case kCloseMessage:
        // Save changes before we close the window
        setLayer(nullptr);
        save_window_pos(this, "LayerProperties");

        deferDelete();
        g_window = nullptr;
        break;

    }
    return Window::onProcessMessage(msg);
  }

  void onStartTimer() {
    if (m_selfUpdate)
      return;

    m_timer.start();
  }

  void onCommitChange() {
    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    m_timer.stop();

    std::string newName = nameValue();
    int newOpacity = opacityValue();
    BlendMode newBlendMode = blendModeValue();

    if (newName != m_layer->name() ||
        newOpacity != m_layer->opacity() ||
        newBlendMode != m_layer->blendMode() ||
        m_userData != m_layer->userData()) {
      try {
        ContextWriter writer(UIContext::instance());
        Transaction transaction(writer.context(), "Set Layer Properties");

        if (newName != m_layer->name())
          transaction.execute(new cmd::SetLayerName(writer.layer(), newName));

        if (newOpacity != m_layer->opacity())
          transaction.execute(new cmd::SetLayerOpacity(static_cast<LayerImage*>(writer.layer()), newOpacity));

        if (newBlendMode != m_layer->blendMode())
          transaction.execute(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(writer.layer()), newBlendMode));

        if (m_userData != m_layer->userData()) {
          transaction.execute(new cmd::SetUserData(writer.layer(), m_userData));

          // Redraw timeline because the layer's user data/color
          // might have changed.
          App::instance()->timeline()->invalidate();
        }

        transaction.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(document());
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setLayer(dynamic_cast<LayerImage*>(const_cast<Layer*>(site.layer())));
    else if (m_layer)
      setLayer(nullptr);
  }

  // DocumentObserver impl
  void onLayerNameChange(DocumentEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onLayerOpacityChange(DocumentEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onLayerBlendModeChange(DocumentEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onPopupUserData() {
    if (m_layer) {
      m_userData = m_layer->userData();
      if (show_user_data_popup(userData()->bounds(), m_userData)) {
        onCommitChange();
      }
    }
  }

  void updateFromLayer() {
    if (m_selfUpdate)
      return;

    m_timer.stop(); // Cancel current editions (just in case)

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    if (m_layer) {
      name()->setText(m_layer->name().c_str());
      name()->setEnabled(true);
      mode()->setSelectedItemIndex((int)m_layer->blendMode());
      mode()->setEnabled(!m_layer->isBackground());
      opacity()->setValue(m_layer->opacity());
      opacity()->setEnabled(!m_layer->isBackground());
      m_userData = m_layer->userData();
    }
    else {
      name()->setText("No Layer");
      name()->setEnabled(false);
      mode()->setEnabled(false);
      opacity()->setEnabled(false);
      m_userData = UserData();
    }
  }

  Timer m_timer;
  LayerImage* m_layer;
  bool m_selfUpdate;
  UserData m_userData;
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command("LayerProperties",
            "Layer Properties",
            CmdRecordableFlag)
{
  m_hasParams = false;
  m_hasName = false;
  m_hasOpacity = false;
  m_opacity = 255;
  m_hasBlendMode = false;
  m_blendMode = doc::BlendMode::NORMAL;
  m_hasVisible = false;
  m_visible = true;
  m_hasUserData = false;
}

void LayerPropertiesCommand::onLoadParams(const Params& params)
{
  m_hasParams = false;
  m_hasName = false;
  m_hasOpacity = false;
  m_hasBlendMode = false;
  m_hasVisible = false;
  m_hasUserData = false;

  if (params.has_param("name")) {
    m_hasName = true;
    m_name = params.get("name");
  }

  if (params.has_param("opacity")) {
    m_hasOpacity = true;
    m_opacity = MID(0, params.get_as<int>("opacity"), 255);
  }

  if (params.has_param("blend-mode")) {
    m_hasBlendMode = true;
    m_blendMode = blend_mode_from_string(params.get("blend-mode"));
  }

  if (params.has_param("visible")) {
    std::string visible = params.get("visible");
    if (visible == "true")
      m_visible = true;
    else if (visible == "false")
      m_visible = false;
    else
      throw std::runtime_error("LayerProperties: visible must be 'true' or 'false'");
    m_hasVisible = true;
  }

  if (params.has_param("user-data")) {
    m_hasUserData = true;
    m_userData = params.get("user-data");
  }

  m_hasParams = m_hasName || m_hasOpacity || m_hasBlendMode ||
                m_hasVisible || m_hasUserData;
}

std::vector<CommandParamInfo> LayerPropertiesCommand::onDescribeParams() const
{
  return {
    {"name", "Layer name. With any property, applies without the window.", false, ""},
    {"opacity", "Layer opacity, 0 to 255.", false, ""},
    {"blend-mode", "Layer blend mode: normal, multiply, screen, overlay, darken, lighten, color-dodge, color-burn, hard-light, soft-light, difference, exclusion, hue, saturation, color or luminosity.", false, ""},
    {"visible", "Layer visibility: true or false.", false, ""},
    {"user-data", "Layer user data text (keeps the current color).", false, ""},
  };
}

bool LayerPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void LayerPropertiesCommand::onExecute(Context* context)
{
  ContextReader reader(context);
  Layer* layer = reader.layer();

  // With the property params, apply them without the window.
  if (m_hasParams) {
    LayerImage* image = dynamic_cast<LayerImage*>(layer);

    if ((m_hasOpacity || m_hasBlendMode) && !image)
      throw std::runtime_error(
        "LayerProperties: opacity and blend-mode need an image layer");

    doc::UserData newUserData = layer->userData();
    if (m_hasUserData)
      newUserData.setText(m_userData);

    bool changeName = m_hasName && (layer->name() != m_name);
    bool changeOpacity = m_hasOpacity && image &&
                         (image->opacity() != m_opacity);
    bool changeBlendMode = m_hasBlendMode && image &&
                           (image->blendMode() != m_blendMode);
    bool changeVisible = m_hasVisible && (layer->isVisible() != m_visible);
    bool changeUserData = m_hasUserData &&
                          (layer->userData() != newUserData);

    if (changeName || changeOpacity || changeBlendMode ||
        changeVisible || changeUserData) {
      ContextWriter writer(reader);
      Transaction transaction(writer.context(), "Set Layer Properties");

      if (changeName)
        transaction.execute(new cmd::SetLayerName(layer, m_name));

      if (changeOpacity)
        transaction.execute(new cmd::SetLayerOpacity(image, m_opacity));

      if (changeBlendMode)
        transaction.execute(new cmd::SetLayerBlendMode(image, m_blendMode));

      if (changeVisible) {
        LayerFlags newFlags = layer->flags();
        if (m_visible)
          newFlags = LayerFlags(int(newFlags) | int(LayerFlags::Visible));
        else
          newFlags = LayerFlags(int(newFlags) & ~int(LayerFlags::Visible));
        transaction.execute(new cmd::SetLayerFlags(layer, newFlags));
      }

      if (changeUserData) {
        transaction.execute(new cmd::SetUserData(layer, newUserData));

        // Redraw timeline because the layer's user data/color
        // might have changed.
        if (App::instance()->timeline())
          App::instance()->timeline()->invalidate();
      }

      transaction.commit();
      update_screen_for_document(writer.document());
    }
    return;
  }

  // The window would block headless.
  if (!App::instance()->isGui()) {
    LOG("LayerProperties: not available in headless mode\n");
    Console console(context);
    console.printf("LayerProperties: not available in headless mode\n");
    return;
  }

  LayerImage* imageLayer = static_cast<LayerImage*>(layer);

  if (!g_window)
    g_window = new LayerPropertiesWindow;

  g_window->setLayer(imageLayer);
  g_window->openWindow();

  // Focus layer name
  g_window->name()->requestFocus();
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
