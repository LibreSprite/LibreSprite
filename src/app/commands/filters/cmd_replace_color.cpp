// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/context.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "base/bind.h"
#include "filters/replace_color_filter.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

static const char* ConfigSection = "ReplaceColor";

// Wrapper for ReplaceColorFilter to handle colors in an easy way
class ReplaceColorFilterWrapper : public ReplaceColorFilter {
public:
  ReplaceColorFilterWrapper(Layer* layer) : m_layer(layer) { }

  void setFrom(const app::Color& from) {
    m_from = from;
    if (m_layer)
      ReplaceColorFilter::setFrom(color_utils::color_for_layer(from, m_layer));
  }
  void setTo(const app::Color& to) {
    m_to = to;
    if (m_layer)
      ReplaceColorFilter::setTo(color_utils::color_for_layer(to, m_layer));
  }

  app::Color getFrom() const { return m_from; }
  app::Color getTo() const { return m_to; }

private:
  Layer* m_layer;
  app::Color m_from;
  app::Color m_to;
};

class ReplaceColorWindow : public FilterWindow {
public:
  ReplaceColorWindow(ReplaceColorFilterWrapper& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Replace Color", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<Widget>("replace_color.xml", "controls"))
    , m_fromButton(app::find_widget<ColorButton>(m_controlsWidget.get(), "from"))
    , m_toButton(app::find_widget<ColorButton>(m_controlsWidget.get(), "to"))
    , m_toleranceSlider(app::find_widget<ui::Slider>(m_controlsWidget.get(), "tolerance"))
  {
    getContainer()->addChild(m_controlsWidget.get());

    m_fromButton->setColor(m_filter.getFrom());
    m_toButton->setColor(m_filter.getTo());
    m_toleranceSlider->setValue(m_filter.getTolerance());

    m_fromButton->Change.connect(&ReplaceColorWindow::onFromChange, this);
    m_toButton->Change.connect(&ReplaceColorWindow::onToChange, this);
    m_toleranceSlider->Change.connect(&ReplaceColorWindow::onToleranceChange, this);
  }

protected:
  void onFromChange(const app::Color& color)
  {
    m_filter.setFrom(color);
    restartPreview();
  }

  void onToChange(const app::Color& color)
  {
    m_filter.setTo(color);
    restartPreview();
  }

  void onToleranceChange()
  {
    m_filter.setTolerance(m_toleranceSlider->getValue());
    restartPreview();
  }

private:
  ReplaceColorFilterWrapper& m_filter;
  std::unique_ptr<ui::Widget> m_controlsWidget;
  ColorButton* m_fromButton;
  ColorButton* m_toButton;
  ui::Slider* m_toleranceSlider;
};

class ReplaceColorCommand : public Command {
public:
  ReplaceColorCommand();
  Command* clone() const override { return new ReplaceColorCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ReplaceColorCommand::ReplaceColorCommand()
  : Command("ReplaceColor",
            "Replace Color",
            CmdRecordableFlag)
{
}

bool ReplaceColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ReplaceColorCommand::onExecute(Context* context)
{
  Site site = context->activeSite();

  ReplaceColorFilterWrapper filter(site.layer());
  filter.setFrom(get_config_color(ConfigSection, "Color1", ColorBar::instance()->getFgColor()));
  filter.setTo(get_config_color(ConfigSection, "Color2", ColorBar::instance()->getBgColor()));
  filter.setTolerance(get_config_int(ConfigSection, "Tolerance", 0));

  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL |
                      TARGET_ALPHA_CHANNEL |
                      TARGET_INDEX_CHANNEL);

  ReplaceColorWindow window(filter, filterMgr);
  if (window.doModal()) {
    set_config_color(ConfigSection, "From", filter.getFrom());
    set_config_color(ConfigSection, "To", filter.getTo());
    set_config_int(ConfigSection, "Tolerance", filter.getTolerance());
  }
}

Command* CommandFactory::createReplaceColorCommand()
{
  return new ReplaceColorCommand;
}

} // namespace app
