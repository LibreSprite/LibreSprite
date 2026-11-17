#pragma once

#include <string>
#include <functional>

namespace app {

  class WriteRealPath {
  public:
    static void createAndShowDialog(std::string mainPath, std::function<void(const std::string&)> onPath);
  };

} // namespace app
