// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "json.hpp"

#include <string>

namespace app {
namespace mcp {

nlohmann::json toolsList();
nlohmann::json callTool(const std::string& name, const nlohmann::json& args);

} // namespace mcp
} // namespace app
