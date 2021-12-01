// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/context.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/pref/preferences.h"
#include "base/bind.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/median_filter.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <stdio.h>

namespace app {

using namespace filters;

static const char* ConfigSection = "Despeckle";

class DespeckleWindow : public FilterWindow {
public:
  DespeckleWindow(MedianFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Median Blur", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<ui::Widget>("despeckle.xml", "controls"))
    , m_widthEntry(app::find_widget<ui::Entry>(m_controlsWidget.get(), "width"))
    , m_heightEntry(app::find_widget<ui::Entry>(m_controlsWidget.get(), "height"))
  {
    getContainer()->addChild(m_controlsWidget.get());

    m_widthEntry->setTextf("%d", m_filter.getWidth());
    m_heightEntry->setTextf("%d", m_filter.getHeight());

    m_widthEntry->Change.connect(&DespeckleWindow::onSizeChange, this);
    m_heightEntry->Change.connect(&DespeckleWindow::onSizeChange, this);
  }

private:
  void onSizeChange()
  {
    m_filter.setSize(m_widthEntry->textInt(),
                     m_heightEntry->textInt());
    restartPreview();
  }

  void setupTiledMode(TiledMode tiledMode)
  {
    m_filter.setTiledMode(tiledMode);
  }

  MedianFilter& m_filter;
  std::unique_ptr<ui::Widget> m_controlsWidget;
  ui::Entry* m_widthEntry;
  ui::Entry* m_heightEntry;
};

//////////////////////////////////////////////////////////////////////
// Despeckle command

class DespeckleCommand : public Command
{
public:
  DespeckleCommand();
  Command* clone() const override { return new DespeckleCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DespeckleCommand::DespeckleCommand()
  : Command("Despeckle",
            "Despeckle",
            CmdRecordableFlag)
{
}

bool DespeckleCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void DespeckleCommand::onExecute(Context* context)
{
  DocumentPreferences& docPref = Preferences::instance()
    .document(context->activeDocument());

  MedianFilter filter;
  filter.setTiledMode((filters::TiledMode)docPref.tiled.mode());
  filter.setSize(get_config_int(ConfigSection, "Width", 3),
                 get_config_int(ConfigSection, "Height", 3));

  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL);

  DespeckleWindow window(filter, filterMgr);
  if (window.doModal()) {
    set_config_int(ConfigSection, "Width", filter.getWidth());
    set_config_int(ConfigSection, "Height", filter.getHeight());
  }
}

Command* CommandFactory::createDespeckleCommand()
{
  return new DespeckleCommand;
}

} // namespace app
