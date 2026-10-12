// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <cstdint>
#include <string>

namespace app {
namespace mcp {

class McpServer {
public:
  static McpServer& instance();

  // Start listening on 127.0.0.1:port (default 64823).
  // Writes mcp-connection.json for the Cursor stdio bridge.
  bool start(uint16_t port = 64823);
  void stop();
  bool isRunning() const;

  uint16_t port() const;
  const std::string& token() const;
  std::string connectionFilePath() const;
  std::string cursorConfigSnippet() const;

private:
  McpServer();
  ~McpServer();
  McpServer(const McpServer&) = delete;
  McpServer& operator=(const McpServer&) = delete;

  struct Impl;
  Impl* m_impl;
};

} // namespace mcp
} // namespace app
