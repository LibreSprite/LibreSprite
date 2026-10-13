// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <functional>
#include <map>
#include <string>

#include "app/task_manager.h"

namespace script {

  // Result of an HTTP request. On failure error is true and reason
  // holds the error message; the other fields are left empty.
  struct HttpResult {
    bool error = false;
    int status = 0;
    std::string reason;
    std::string body;
    std::map<std::string, std::string> headers;
  };

  // Performs a blocking HTTP request on a TaskManager worker thread
  // and delivers the result on the main thread. method defaults to
  // GET; body is only sent with POST/PUT/DELETE. A default User-Agent
  // and Content-Type are added when not provided.
  app::TaskHandle httpFetch(const std::string& url,
                            const std::string& method,
                            const std::string& body,
                            bool isBinaryBody,
                            const std::map<std::string, std::string>& headers,
                            std::function<void(HttpResult&&)>&& cb);

}
