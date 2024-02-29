// Aseprite Network Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef NET_HTTP_REQUEST_H_INCLUDED
#define NET_HTTP_REQUEST_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace net {

using HttpHeaders = std::unordered_map<std::string, std::string>;
class HttpRequestImpl;
class HttpResponse;

class HttpRequest {
public:
  HttpRequest(const std::string& url);
  ~HttpRequest();

  void setHeaders(const HttpHeaders& headers);
  void setPostBody(const std::string& body);
  bool send(HttpResponse& response);
  void abort();

private:
  std::unique_ptr<HttpRequestImpl> m_impl;

  DISABLE_COPYING(HttpRequest);
};

} // namespace net

#endif
