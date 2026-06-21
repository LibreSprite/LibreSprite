#include "app/commands/cmd_write_real_path.h"
#include "base/bind.h"
#include "ui/ui.h"

namespace app {

void WriteRealPath::createAndShowDialog(std::string mainPath, std::function<void(const std::string&)> onPath)
{
  auto window = std::make_unique<ui::Window>(ui::Window::WithTitleBar, "Write Your Own Path");

  auto box = new ui::Box(ui::VERTICAL);
  auto label = new ui::Label("Enter the full path:");
  auto entry = new ui::Entry(256, mainPath.c_str());
  auto btnBox = new ui::Box(ui::HORIZONTAL | ui::HOMOGENEOUS);
  auto btnOk = new ui::Button("&OK");
  auto btnCancel = new ui::Button("&Cancel");

  btnCancel->Click.connect(base::Bind<void>(&ui::Window::closeWindow, window.get(), btnCancel));
  btnOk->Click.connect([&]{
    onPath(entry->text());
    window->closeWindow(btnOk);
  });

  btnBox->addChild(btnOk);
  btnBox->addChild(btnCancel);

  box->addChild(label);
  box->addChild(entry);
  box->addChild(btnBox);

  window->addChild(box);
  window->remapWindow();
  window->centerWindow();
  window->openWindowInForeground();
}

} // namespace app
