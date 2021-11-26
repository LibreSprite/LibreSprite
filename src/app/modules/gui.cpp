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
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/document.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/tools/ink.h"
#include "app/tools/tool_box.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/memory.h"
#include "base/unique_ptr.h"
#include "doc/sprite.h"
#include "she/display.h"
#include "she/error.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <algorithm>
#include <list>
#include <memory>
#include <vector>

namespace app {

using namespace gfx;
using namespace ui;
using namespace app::skin;

static struct {
  int width;
  int height;
  int scale;
} try_resolutions[] = { { 1024, 768, 2 },
                        {  800, 600, 2 },
                        {  640, 480, 2 },
                        {  320, 240, 1 },
                        {  320, 200, 1 },
                        {    0,   0, 0 } };

//////////////////////////////////////////////////////////////////////

class CustomizedGuiManager : public Manager, public LayoutIO {
protected:
  bool onProcessMessage(Message* msg) override;
  LayoutIO* onGetLayoutIO() override { return this; }
  void onNewDisplayConfiguration() override;

  // LayoutIO implementation
  std::string loadLayout(Widget* widget) override;
  void saveLayout(Widget* widget, const std::string& str) override;
};

Widget::Shared<CustomizedGuiManager> _cgm{"GuiManager"};

static she::Display* main_display = NULL;
static std::shared_ptr<CustomizedGuiManager> manager;
static Theme* gui_theme = NULL;

static inject<ui::Timer> defered_invalid_timer{nullptr};
static gfx::Region defered_invalid_region;

// Load & save graphics configuration
static void load_gui_config(int& w, int& h, bool& maximized,
                            std::string& windowLayout);
static void save_gui_config();

static bool create_main_display(bool gpuAccel,
                                bool& maximized,
                                std::string& lastError)
{
  int w, h;
  std::string windowLayout;
  load_gui_config(w, h, maximized, windowLayout);

  // Scale is equal to 0 when it's the first time the program is
  // executed.
  int scale = Preferences::instance().general.screenScale();

  she::instance()->setGpuAcceleration(gpuAccel);

  try {
    if (w > 0 && h > 0) {
      main_display = she::instance()->createDisplay(
        w, h, (scale == 0 ? 2: MID(1, scale, 4)));
    }
  }
  catch (const she::DisplayCreationException& e) {
    lastError = e.what();
  }

  if (!main_display) {
    for (int c=0; try_resolutions[c].width; ++c) {
      try {
        main_display =
          she::instance()->createDisplay(
            try_resolutions[c].width,
            try_resolutions[c].height,
            (scale == 0 ? try_resolutions[c].scale: scale));
        break;
      }
      catch (const she::DisplayCreationException& e) {
        lastError = e.what();
      }
    }
  }

  if (main_display) {
    // Change the scale value only in the first run (this will be
    // saved when the program is closed).
    if (scale == 0)
      Preferences::instance().general.screenScale(main_display->scale());

    ResourceFinder rf;
    rf.includeDataDir("icons/ase64.png");
    if (rf.findFirst())
        main_display->setIcon(she::instance()->loadRgbaSurface(rf.filename().c_str()));

    if (!windowLayout.empty()) {
      main_display->setLayout(windowLayout);
      if (main_display->isMinimized())
        main_display->maximize();
    }
  }

  return (main_display != nullptr);
}

// Initializes GUI.
int init_module_gui()
{
  bool maximized = false;
  std::string lastError = "Unknown error";
  bool gpuAccel = Preferences::instance().general.gpuAcceleration();

  if (!create_main_display(gpuAccel, maximized, lastError)) {
    // If we've created the display with hardware acceleration,
    // now we try to do it without hardware acceleration.
    if (gpuAccel &&
        (int(she::instance()->capabilities()) &
         int(she::Capabilities::GpuAccelerationSwitch)) == int(she::Capabilities::GpuAccelerationSwitch)) {
      if (create_main_display(false, maximized, lastError)) {
        // Disable hardware acceleration
        Preferences::instance().general.gpuAcceleration(false);
      }
    }
  }

  if (!main_display) {
    she::error_message(
      ("Unable to create a user-interface display.\nDetails: "+lastError+"\n").c_str());
    return -1;
  }

  manager = inject<Widget>{"GuiManager"};
  manager->setDisplay(main_display);

  // Setup the GUI theme for all widgets
  gui_theme = new SkinTheme();
  gui_theme->setScale(Preferences::instance().experimental.uiScale());
  CurrentTheme::set(gui_theme);

  if (maximized)
    main_display->maximize();

  // Set graphics options for next time
  save_gui_config();

  return 0;
}

void exit_module_gui()
{
  save_gui_config();

  defered_invalid_timer.reset();
  manager.reset();

  // Now we can destroy theme
  CurrentTheme::set(NULL);
  delete gui_theme;

  main_display->dispose();
}

static void load_gui_config(int& w, int& h, bool& maximized,
                            std::string& windowLayout)
{
  gfx::Size defSize = she::instance()->defaultNewDisplaySize();

  w = get_config_int("GfxMode", "Width", defSize.w);
  h = get_config_int("GfxMode", "Height", defSize.h);
  maximized = get_config_bool("GfxMode", "Maximized", false);
  windowLayout = get_config_string("GfxMode", "WindowLayout", "");
}

static void save_gui_config()
{
  she::Display* display = manager->getDisplay();
  if (display) {
    set_config_bool("GfxMode", "Maximized", display->isMaximized());
    set_config_int("GfxMode", "Width", display->originalWidth());
    set_config_int("GfxMode", "Height", display->originalHeight());

    std::string windowLayout = display->getLayout();
    if (!windowLayout.empty())
      set_config_string("GfxMode", "WindowLayout", windowLayout.c_str());
  }
}

void update_screen_for_document(const Document* document)
{
  // Without document.
  if (!document) {
    // Well, change to the default palette.
    if (set_current_palette(NULL, false)) {
      // If the palette changes, refresh the whole screen.
      if (manager)
        manager->invalidate();
    }
  }
  // With a document.
  else {
    const_cast<Document*>(document)->notifyGeneralUpdate();

    // Update the tabs (maybe the modified status has been changed).
    app_rebuild_documents_tabs();
  }
}

void load_window_pos(Widget* window, const char *section)
{
  // Default position
  Rect orig_pos = window->bounds();
  Rect pos = orig_pos;

  // Load configurated position
  pos = get_config_rect(section, "WindowPos", pos);

  pos.w = MID(orig_pos.w, pos.w, ui::display_w());
  pos.h = MID(orig_pos.h, pos.h, ui::display_h());

  pos.setOrigin(Point(MID(0, pos.x, ui::display_w()-pos.w),
      MID(0, pos.y, ui::display_h()-pos.h)));

  window->setBounds(pos);
}

void save_window_pos(Widget* window, const char *section)
{
  set_config_rect(section, "WindowPos", window->bounds());
}

Widget* setup_mini_font(Widget* widget)
{
  SkinPropertyPtr skinProp = get_skin_property(widget);
  skinProp->setMiniFont();
  return widget;
}

Widget* setup_mini_look(Widget* widget)
{
  return setup_look(widget, MiniLook);
}

Widget* setup_look(Widget* widget, LookType lookType)
{
  SkinPropertyPtr skinProp = get_skin_property(widget);
  skinProp->setLook(lookType);
  return widget;
}

void setup_bevels(Widget* widget, int b1, int b2, int b3, int b4)
{
  SkinPropertyPtr skinProp = get_skin_property(widget);
  skinProp->setUpperLeft(b1);
  skinProp->setUpperRight(b2);
  skinProp->setLowerLeft(b3);
  skinProp->setLowerRight(b4);
}

//////////////////////////////////////////////////////////////////////
// Button style (convert radio or check buttons and draw it like
// normal buttons)

CheckBox* check_button_new(const char *text, int b1, int b2, int b3, int b4)
{
  CheckBox* widget = new CheckBox(text, kButtonWidget);

  widget->setAlign(CENTER | MIDDLE);

  setup_mini_look(widget);
  setup_bevels(widget, b1, b2, b3, b4);
  return widget;
}

void defer_invalid_rect(const gfx::Rect& rc)
{
  if (!defered_invalid_timer)
    defered_invalid_timer = ui::Timer::create(250, *manager.get());

  defered_invalid_timer->stop();
  defered_invalid_timer->start();
  defered_invalid_region.createUnion(defered_invalid_region, gfx::Region(rc));
}

// Manager event handler.
bool CustomizedGuiManager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseDisplayMessage:
      {
        // Execute the "Exit" command.
        Command* command = CommandsModule::instance()->getCommandByName(CommandId::Exit);
        UIContext::instance()->executeCommand(command);
      }
      break;

    case kDropFilesMessage:
      {
        const DropFilesMessage::Files& files = static_cast<DropFilesMessage*>(msg)->files();

        // Open all files
        Command* cmd_open_file =
          CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
        Params params;

        UIContext* ctx = UIContext::instance();

        for (const auto& fn : files) {
          // If the document is already open, select it.
          Document* doc = static_cast<Document*>(ctx->documents().getByFileName(fn));
          if (doc) {
            DocumentView* docView = ctx->getFirstDocumentView(doc);
            if (docView)
              ctx->setActiveView(docView);
            else {
              ASSERT(false);    // Must be some DocumentView available
            }
          }
          // Load the file
          else {
            params.set("filename", fn.c_str());
            ctx->executeCommand(cmd_open_file, params);
          }
        }
      }
      break;

    case kKeyDownMessage: {
#ifdef _DEBUG
      // Left Shift+Ctrl+Q generates a crash (useful to test the anticrash feature)
      if (msg->ctrlPressed() &&
          msg->shiftPressed() &&
          static_cast<KeyMessage*>(msg)->scancode() == kKeyQ) {
        int* p = nullptr;
        *p = 0;
      }
#endif

      // Call base impl to check if there is a foreground window as
      // top level that needs keys. (In this way we just do not
      // process keyboard shortcuts for menus and tools).
      if (Manager::onProcessMessage(msg))
        return true;

      for (const Key* key : *KeyboardShortcuts::instance()) {
        if (key->isPressed(msg)) {
          // Cancel menu-bar loops (to close any popup menu)
          App::instance()->mainWindow()->getMenuBar()->cancelMenuLoop();

          switch (key->type()) {

            case KeyType::Tool: {
              tools::Tool* current_tool = App::instance()->activeTool();
              tools::Tool* select_this_tool = key->tool();
              tools::ToolBox* toolbox = App::instance()->toolBox();
              std::vector<tools::Tool*> possibles;

              // Collect all tools with the pressed keyboard-shortcut
              for (tools::Tool* tool : *toolbox) {
                Key* key = KeyboardShortcuts::instance()->tool(tool);
                if (key && key->isPressed(msg))
                  possibles.push_back(tool);
              }

              if (possibles.size() >= 2) {
                bool done = false;

                for (size_t i=0; i<possibles.size(); ++i) {
                  if (possibles[i] != current_tool &&
                      ToolBar::instance()->isToolVisible(possibles[i])) {
                    select_this_tool = possibles[i];
                    done = true;
                    break;
                  }
                }

                if (!done) {
                  for (size_t i=0; i<possibles.size(); ++i) {
                    // If one of the possibilities is the current tool
                    if (possibles[i] == current_tool) {
                      // We select the next tool in the possibilities
                      select_this_tool = possibles[(i+1) % possibles.size()];
                      break;
                    }
                  }
                }
              }

              ToolBar::instance()->selectTool(select_this_tool);
              return true;
            }

            case KeyType::Command: {
              Command* command = key->command();

              // Commands are executed only when the main window is
              // the current window running at foreground.
              for (auto childWidget : children()) {
                Window* child = static_cast<Window*>(childWidget);

                // There are a foreground window executing?
                if (child->isForeground()) {
                  break;
                }
                // Is it the desktop and the top-window=
                else if (child->isDesktop() && child == App::instance()->mainWindow()) {
                  // OK, so we can execute the command represented
                  // by the pressed-key in the message...
                  UIContext::instance()->executeCommand(
                    command, key->params());
                  return true;
                }
              }
              break;
            }

            case KeyType::Quicktool: {
              // Do nothing, it is used in the editor through the
              // KeyboardShortcuts::getCurrentQuicktool() function.
              break;
            }

          }
          break;
        }
      }
      break;
    }

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer().get() == defered_invalid_timer) {
        invalidateDisplayRegion(defered_invalid_region);
        defered_invalid_region.clear();
        defered_invalid_timer->stop();
      }
      break;

  }

  return Manager::onProcessMessage(msg);
}

void CustomizedGuiManager::onNewDisplayConfiguration()
{
  Manager::onNewDisplayConfiguration();
  save_gui_config();
}

std::string CustomizedGuiManager::loadLayout(Widget* widget)
{
  if (widget->window() == nullptr)
    return "";

  std::string windowId = widget->window()->id();
  std::string widgetId = widget->id();

  return get_config_string(("layout:"+windowId).c_str(), widgetId.c_str(), "");
}

void CustomizedGuiManager::saveLayout(Widget* widget, const std::string& str)
{
  if (widget->window() == NULL)
    return;

  std::string windowId = widget->window()->id();
  std::string widgetId = widget->id();

  set_config_string(("layout:"+windowId).c_str(),
                    widgetId.c_str(),
                    str.c_str());
}

} // namespace app
