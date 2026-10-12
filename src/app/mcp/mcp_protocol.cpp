// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/mcp/mcp_protocol.h"
#include "app/mcp/mcp_tools.h"

namespace app {
namespace mcp {
namespace {

using nlohmann::json;

json makeError(const json& id, int code, const std::string& message)
{
  return {
    {"jsonrpc", "2.0"},
    {"id", id},
    {"error", {{"code", code}, {"message", message}}}
  };
}

json makeResult(const json& id, json result)
{
  return {
    {"jsonrpc", "2.0"},
    {"id", id},
    {"result", std::move(result)}
  };
}

json handleInitialize(const json& /*params*/)
{
  return {
    {"protocolVersion", "2024-11-05"},
    {"capabilities", {
      {"tools", json::object()}
    }},
    {"serverInfo", {
      {"name", "libresprite-native-mcp"},
      {"version", "0.1.0"}
    }}
  };
}

} // namespace

std::optional<json> handleMessage(const json& message)
{
  if (!message.is_object())
    return makeError(nullptr, -32600, "Invalid Request");

  const std::string method = message.value("method", "");
  const bool hasId = message.contains("id");
  json id = hasId ? message["id"] : json(nullptr);

  try {
    if (method == "initialize") {
      if (!hasId) return std::nullopt;
      return makeResult(id, handleInitialize(message.value("params", json::object())));
    }
    if (method == "notifications/initialized" || method == "initialized") {
      return std::nullopt;
    }
    if (method == "ping") {
      if (!hasId) return std::nullopt;
      return makeResult(id, json::object());
    }
    if (method == "tools/list") {
      if (!hasId) return std::nullopt;
      return makeResult(id, toolsList());
    }
    if (method == "tools/call") {
      if (!hasId) return std::nullopt;
      json params = message.value("params", json::object());
      std::string name = params.value("name", "");
      json args = params.value("arguments", json::object());
      if (name.empty())
        return makeError(id, -32602, "Missing tool name");
      return makeResult(id, callTool(name, args));
    }

    if (!hasId)
      return std::nullopt;
    return makeError(id, -32601, "Method not found: " + method);
  }
  catch (const std::exception& e) {
    if (!hasId) return std::nullopt;
    return makeError(id, -32000, e.what());
  }
}

} // namespace mcp
} // namespace app
