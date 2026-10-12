// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <functional>
#include <string>

namespace app {
namespace mcp {

// Call once from the UI thread when the MCP server starts.
void initUiBridge();

// Run work on the UI/gfx thread and wait for completion.
// Returns an empty string on success, or an error message.
std::string callOnUiThread(const std::function<void()>& fn);

} // namespace mcp
} // namespace app
