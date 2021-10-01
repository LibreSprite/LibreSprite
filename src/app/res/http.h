// LibreSprite
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <chrono>
#include <sstream>
#include "app/task_manager.h"
#include "net/http_request.h"
#include "net/http_response.h"

namespace app {
  class HTTP {
  public:
    struct Result {
      std::string body;
      int status;
    };

    static TaskHandle get(const std::string& url, std::function<void(Result&&)>&& callback) {
      auto req = std::make_shared<net::HttpRequest>(url);
      return TaskManager::instance().addTask<Result>(
        [=]{
          Result result;
          std::stringstream ss;
          net::HttpResponse res{&ss};
          result.status = req->send(res);
          if (result.status)
            result.status = res.status();
          result.body = ss.str();
          return result;
        },
        std::move(callback),
        [req]{req->abort();});
    }
  };
}
