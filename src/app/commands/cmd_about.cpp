// Aseprite    - Copyright (C) 2001-2015  David Capello
// LibreSprite - Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "base/bind.h"
#include "ui/ui.h"

#include <memory>

namespace app {

using namespace ui;

class AboutCommand : public Command {
public:
  AboutCommand();
  Command* clone() const override { return new AboutCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

AboutCommand::AboutCommand()
  : Command("About",
            "About",
            CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  std::unique_ptr<Window> window = std::make_unique<Window>(Window::WithTitleBar, "About " PACKAGE);
  std::shared_ptr<Box> box1 = std::make_shared<Box>(VERTICAL);
  std::shared_ptr<Grid> grid = std::make_shared<Grid>(2, false);
  std::shared_ptr<Label> title = std::make_shared<Label>(PACKAGE_AND_VERSION);
  std::shared_ptr<Label> subtitle = std::make_shared<Label>("Animated sprite editor & pixel art tool");
  std::shared_ptr<Separator> authors_separator1 = std::make_shared<Separator>("Authors:", HORIZONTAL | TOP);
  std::shared_ptr<Separator> authors_separator2 = std::make_shared<Separator>("", HORIZONTAL);
  std::shared_ptr<Label> author1 = std::make_shared<LinkLabel>("http://davidcapello.com/", "David Capello");
  std::shared_ptr<Label> author1_desc = std::make_shared<Label>("- Lead developer, graphics & maintainer");
  std::shared_ptr<Label> author2 = std::make_shared<LinkLabel>("http://ilkke.blogspot.com/", "Ilija Melentijevic");
  std::shared_ptr<Label> author2_desc = std::make_shared<Label>("- Default skin & graphics introduced in v0.8");
  std::shared_ptr<Label> author3 = std::make_shared<LinkLabel>(WEBSITE_CONTRIBUTORS, "Contributors");
  std::shared_ptr<Box> bottom_box1 = std::make_shared<Box>(HORIZONTAL);
  std::shared_ptr<Box> bottom_box2 = std::make_shared<Box>(HORIZONTAL);
  std::shared_ptr<Box> bottom_box3 = std::make_shared<Box>(HORIZONTAL);
  std::shared_ptr<Label> copyright = std::make_shared<Label>(COPYRIGHT);
  std::shared_ptr<Label> website = std::make_shared<LinkLabel>(WEBSITE);
  std::shared_ptr<Button> close_button = std::make_shared<Button>("&Close");

  grid->addChildInCell(title, 2, 1, 0);
  grid->addChildInCell(subtitle, 2, 1, 0);
  grid->addChildInCell(authors_separator1, 2, 1, 0);
  grid->addChildInCell(author1, 1, 1, 0);
  grid->addChildInCell(author1_desc, 1, 1, 0);
  grid->addChildInCell(author2, 1, 1, 0);
  grid->addChildInCell(author2_desc, 1, 1, 0);
  grid->addChildInCell(author3, 2, 1, 0);
  grid->addChildInCell(authors_separator2, 2, 1, 0);
  grid->addChildInCell(copyright, 2, 1, 0);
  grid->addChildInCell(website, 2, 1, 0);
  grid->addChildInCell(bottom_box1, 2, 1, 0);

  close_button->setFocusMagnet(true);

  bottom_box2->setExpansive(true);
  bottom_box3->setExpansive(true);

  bottom_box1->addChild(bottom_box2);
  bottom_box1->addChild(close_button);
  bottom_box1->addChild(bottom_box3);

  box1->addChild(grid);
  window->addChild(box1);

  close_button->setBorder(
    gfx::Border(
      close_button->border().left() + 16*guiscale(),
      close_button->border().top(),
      close_button->border().right() + 16*guiscale(),
      close_button->border().bottom()));

  close_button->Click.connect(base::Bind<void>(&Window::closeWindow, window.get(), close_button.get()));

  window->openWindowInForeground();
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
