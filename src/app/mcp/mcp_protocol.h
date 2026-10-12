// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "json.hpp"

#include <optional>
#include <string>

namespace app {
namespace mcp {

// Process one JSON-RPC MCP message. Returns a response object when the
// message is a request; std::nullopt for notifications.
std::optional<nlohmann::json> handleMessage(const nlohmann::json& message);

} // namespace mcp
} // namespace app
