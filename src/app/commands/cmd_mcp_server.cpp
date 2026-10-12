// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/mcp/mcp_server.h"
#include "ui/alert.h"

#include <sstream>

namespace app {

class McpServerCommand : public Command {
public:
  McpServerCommand();
  Command* clone() const override { return new McpServerCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

McpServerCommand::McpServerCommand()
  : Command("McpServer",
            "MCP Server",
            CmdUIOnlyFlag)
{
}

void McpServerCommand::onExecute(Context* context)
{
  (void)context;
  auto& server = mcp::McpServer::instance();

  if (!server.isRunning()) {
    int choice = ui::Alert::show(
      "MCP Server"
      "<<Start the native MCP server on localhost?"
      "<<Cursor can connect via tools/mcp_stdio_bridge.py"
      "||&Start||&Cancel");
    if (choice != 1)
      return;

    if (!server.start(64823)) {
      ui::Alert::show(
        "MCP Server"
        "<<Failed to bind 127.0.0.1:64823"
        "<<Is another process using that port?"
        "||&OK");
      return;
    }

    std::ostringstream msg;
    msg << "MCP Server"
        << "<<Running on 127.0.0.1:" << server.port()
        << "<<Connection file:"
        << "<<" << server.connectionFilePath()
        << "<<Token: " << server.token()
        << "||&OK";
    ui::Alert::show(msg.str().c_str());
    return;
  }

  std::ostringstream msg;
  msg << "MCP Server"
      << "<<Status: running"
      << "<<Address: 127.0.0.1:" << server.port()
      << "<<Connection file:"
      << "<<" << server.connectionFilePath()
      << "<<Token: " << server.token()
      << "<<Use tools/mcp_stdio_bridge.py in Cursor mcp.json"
      << "||&Stop||&Keep Running";
  int choice = ui::Alert::show(msg.str().c_str());
  if (choice == 1)
    server.stop();
}

Command* CommandFactory::createMcpServerCommand()
{
  return new McpServerCommand;
}

} // namespace app
