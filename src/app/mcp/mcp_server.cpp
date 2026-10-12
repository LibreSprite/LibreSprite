// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/mcp/mcp_server.h"
#include "app/mcp/mcp_protocol.h"
#include "app/mcp/mcp_ui_bridge.h"

#include "app/resource_finder.h"
#include "json.hpp"

#include <atomic>
#include <cctype>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32)
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#else
  #error "Native MCP server currently supports POSIX sockets only"
#endif

namespace app {
namespace mcp {
namespace {

using nlohmann::json;

std::string randomToken()
{
  static constexpr char kChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(kChars) - 2));
  std::string out;
  out.reserve(32);
  for (int i = 0; i < 32; ++i)
    out.push_back(kChars[dist(rng)]);
  return out;
}

bool sendAll(int fd, const char* data, size_t len)
{
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = ::send(fd, data + sent, len - sent, 0);
    if (n <= 0) return false;
    sent += static_cast<size_t>(n);
  }
  return true;
}

bool recvUntil(int fd, std::string& out, const std::string& delim, size_t maxBytes)
{
  char buf[1024];
  while (out.find(delim) == std::string::npos) {
    if (out.size() >= maxBytes) return false;
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) return false;
    out.append(buf, buf + n);
  }
  return true;
}

void writeHttpResponse(int fd, int status, const std::string& statusText,
                       const std::string& body, const std::string& contentType = "application/json")
{
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status << " " << statusText << "\r\n"
      << "Content-Type: " << contentType << "\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;
  std::string resp = oss.str();
  sendAll(fd, resp.data(), resp.size());
}

} // namespace

struct McpServer::Impl {
  std::atomic<bool> running{false};
  std::atomic<bool> stopRequested{false};
  uint16_t port = 64823;
  std::string token;
  std::string connectionPath;
  int listenFd = -1;
  std::thread worker;

  void writeConnectionFile()
  {
    ResourceFinder rf;
    rf.includeUserDir("mcp-connection.json");
    connectionPath = rf.getFirstOrCreateDefault();

    json doc = {
      {"host", "127.0.0.1"},
      {"port", port},
      {"token", token},
      {"url", "http://127.0.0.1:" + std::to_string(port) + "/mcp"}
    };

    std::ofstream out(connectionPath.c_str(), std::ios::binary | std::ios::trunc);
    out << doc.dump(2) << "\n";
  }

  void handleClient(int clientFd)
  {
    std::string raw;
    if (!recvUntil(clientFd, raw, "\r\n\r\n", 1 << 20)) {
      ::close(clientFd);
      return;
    }

    size_t headerEnd = raw.find("\r\n\r\n");
    std::string headers = raw.substr(0, headerEnd);
    std::string body = raw.substr(headerEnd + 4);

    std::istringstream hs(headers);
    std::string requestLine;
    std::getline(hs, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r')
      requestLine.pop_back();

    std::string method, path, version;
    {
      std::istringstream rl(requestLine);
      rl >> method >> path >> version;
    }

    size_t contentLength = 0;
    std::string auth;
    std::string line;
    while (std::getline(hs, line)) {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      auto colon = line.find(':');
      if (colon == std::string::npos) continue;
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      while (!value.empty() && value[0] == ' ') value.erase(value.begin());
      for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if (key == "content-length") {
        contentLength = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
      }
      else if (key == "authorization") {
        auth = value;
      }
    }

    while (body.size() < contentLength) {
      char buf[4096];
      size_t need = contentLength - body.size();
      ssize_t n = ::recv(clientFd, buf, need > sizeof(buf) ? sizeof(buf) : need, 0);
      if (n <= 0) {
        ::close(clientFd);
        return;
      }
      body.append(buf, buf + n);
    }

    if (method == "GET" && (path == "/health" || path == "/")) {
      writeHttpResponse(clientFd, 200, "OK",
        json({{"ok", true}, {"server", "libresprite-native-mcp"}}).dump());
      ::close(clientFd);
      return;
    }

    const std::string expected = "Bearer " + token;
    if (auth != expected) {
      writeHttpResponse(clientFd, 401, "Unauthorized",
        json({{"error", "unauthorized"}}).dump());
      ::close(clientFd);
      return;
    }

    if (method != "POST" || (path != "/mcp" && path != "/")) {
      writeHttpResponse(clientFd, 404, "Not Found",
        json({{"error", "not found"}}).dump());
      ::close(clientFd);
      return;
    }

    try {
      json msg = json::parse(body.empty() ? "null" : body);
      if (msg.is_array()) {
        json responses = json::array();
        for (const auto& item : msg) {
          auto resp = handleMessage(item);
          if (resp) responses.push_back(*resp);
        }
        writeHttpResponse(clientFd, 200, "OK", responses.dump());
      }
      else {
        auto resp = handleMessage(msg);
        if (resp)
          writeHttpResponse(clientFd, 200, "OK", resp->dump());
        else
          writeHttpResponse(clientFd, 202, "Accepted", "");
      }
    }
    catch (const std::exception& e) {
      writeHttpResponse(clientFd, 400, "Bad Request",
        json({{"jsonrpc", "2.0"},
              {"id", nullptr},
              {"error", {{"code", -32700}, {"message", e.what()}}}}).dump());
    }

    ::close(clientFd);
  }

  void runLoop()
  {
    while (!stopRequested.load()) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(listenFd, &fds);
      timeval tv{};
      tv.tv_sec = 0;
      tv.tv_usec = 250000;
      int ready = ::select(listenFd + 1, &fds, nullptr, nullptr, &tv);
      if (ready < 0) {
        if (errno == EINTR) continue;
        break;
      }
      if (ready == 0) continue;

      sockaddr_in clientAddr{};
      socklen_t clientLen = sizeof(clientAddr);
      int clientFd = ::accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
      if (clientFd < 0) continue;

      // Only accept localhost connections (extra safety beyond bind).
      if (clientAddr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        ::close(clientFd);
        continue;
      }

      handleClient(clientFd);
    }

    if (listenFd >= 0) {
      ::close(listenFd);
      listenFd = -1;
    }
    running = false;
  }
};

McpServer::McpServer()
  : m_impl(new Impl)
{
}

McpServer::~McpServer()
{
  stop();
  delete m_impl;
}

McpServer& McpServer::instance()
{
  static McpServer server;
  return server;
}

bool McpServer::start(uint16_t port)
{
  if (m_impl->running.load())
    return true;

  m_impl->port = port ? port : 64823;
  m_impl->token = randomToken();
  m_impl->stopRequested = false;

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return false;

  int yes = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_impl->port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(fd);
    return false;
  }
  if (::listen(fd, 16) < 0) {
    ::close(fd);
    return false;
  }

  m_impl->listenFd = fd;
  m_impl->writeConnectionFile();
  initUiBridge();
  m_impl->running = true;
  m_impl->worker = std::thread([this]{ m_impl->runLoop(); });
  return true;
}

void McpServer::stop()
{
  if (!m_impl->running.load() && !m_impl->worker.joinable())
    return;

  m_impl->stopRequested = true;
  if (m_impl->listenFd >= 0) {
    // Wake select by connecting then closing.
    int wake = ::socket(AF_INET, SOCK_STREAM, 0);
    if (wake >= 0) {
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(m_impl->port);
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      ::connect(wake, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
      ::close(wake);
    }
  }
  if (m_impl->worker.joinable())
    m_impl->worker.join();
  m_impl->running = false;
}

bool McpServer::isRunning() const
{
  return m_impl->running.load();
}

uint16_t McpServer::port() const
{
  return m_impl->port;
}

const std::string& McpServer::token() const
{
  return m_impl->token;
}

std::string McpServer::connectionFilePath() const
{
  return m_impl->connectionPath;
}

std::string McpServer::cursorConfigSnippet() const
{
  // Prefer pointing Cursor at the bridge script shipped with the repo/build.
  return std::string(
    "{\n"
    "  \"mcpServers\": {\n"
    "    \"libresprite\": {\n"
    "      \"command\": \"python3\",\n"
    "      \"args\": [\"") +
    // Absolute path is filled by docs; bridge auto-discovers connection file.
    "tools/mcp_stdio_bridge.py\"]\n"
    "    }\n"
    "  }\n"
    "}\n";
}

} // namespace mcp
} // namespace app
