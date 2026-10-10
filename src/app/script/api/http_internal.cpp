// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/script/api/http_internal.h"

// The browser has no sockets: the web build uses the fetch() implementation
// from http_internal_wasm.cpp instead.
#ifndef __EMSCRIPTEN__

#include "delta/StringUtils.hpp"
#include "httplib/httplib.h"

#include <regex>

namespace {

  // Splits "scheme://host[:port][/path]" into its parts.
  bool parseUrl(const std::string& url, std::string& protocol, std::string& host, std::string& port, std::string& path) {
    static const std::regex re(
      R"+(^(https?)://([^:/]+)(?::([0-9]+))?(.*)$)+",
      std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(url, m, re))
      return false;
    protocol = su::toLower(m.str(1));
    host = su::toLower(m.str(2));
    port = m.str(3);
    path = m.str(4);
    if (port.empty())
      port = protocol == "http" ? "80" : "443";
    return true;
  }

  // Performs the request (blocking; runs on a TaskManager worker).
  script::HttpResult performRequest(httplib::Client& cli, const std::string& path, const std::string& method,
                                    const std::string& body, bool isBinaryBody,
                                    const std::map<std::string, std::string>& headers) {
    script::HttpResult result;

    // Build headers: user headers + default User-Agent (user overrides default)
    httplib::Headers reqHeaders;
    bool hasUserAgent = false;
    bool hasContentType = false;
    for (auto& [key, val] : headers) {
      if (su::toLower(key) == "user-agent")
        hasUserAgent = true;
      if (su::toLower(key) == "content-type")
        hasContentType = true;
      reqHeaders.emplace(key, val);
    }
    if (!hasUserAgent)
      reqHeaders.emplace("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    // Content-Type for request bodies
    std::string contentType =
      isBinaryBody && !hasContentType ? "application/octet-stream" : "application/json";

    httplib::Result res;
    if (method == "POST") {
      res = cli.Post(path, reqHeaders, body, contentType);
    } else if (method == "PUT") {
      res = cli.Put(path, reqHeaders, body, contentType);
    } else if (method == "DELETE") {
      res = body.empty() ? cli.Delete(path, reqHeaders)
                         : cli.Delete(path, reqHeaders, body, contentType);
    } else {
      res = cli.Get(path, reqHeaders);
    }

    if (res) {
      result.status = res->status;
      result.reason = res->reason;
      result.body = std::move(res->body);
      for (auto& [key, val] : res->headers)
        result.headers[key] = val;
    } else {
      result.error = true;
      result.reason = httplib::to_string(res.error());
    }
    return result;
  }

} // namespace

namespace script {

  app::TaskHandle httpFetch(const std::string& url, const std::string& methodIn, const std::string& body,
                            bool isBinaryBody, const std::map<std::string, std::string>& headers,
                            std::function<void(HttpResult&&)>&& cb) {
    std::string protocol, host, port, path;
    std::string method = methodIn.empty() ? "GET" : su::toUpper(methodIn);
    std::shared_ptr<httplib::Client> cli;
    std::string parseError;
    if (parseUrl(url, protocol, host, port, path))
      cli = std::make_shared<httplib::Client>(protocol + "://" + host + ":" + port);
    else
      parseError = "Invalid URL";

    return app::TaskManager::instance().addTask<HttpResult>(
      [cli, path, method, body, isBinaryBody, headers, parseError]{
        if (!cli) {
          HttpResult result;
          result.error = true;
          result.reason = parseError;
          return result;
        }
        try {
          return performRequest(*cli, path, method, body, isBinaryBody, headers);
        } catch (const std::exception& e) {
          HttpResult result;
          result.error = true;
          result.reason = e.what();
          return result;
        }
      },
      std::move(cb),
      [cli]{ if (cli) cli->stop(); });
  }

}

#endif // __EMSCRIPTEN__
