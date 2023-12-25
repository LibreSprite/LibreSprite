// LibreSprite
// Copyright (c) 2024 LibreSprite Contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "net/http_request.h"

#include "base/debug.h"
#include "net/http_response.h"

#if !__has_include(<curl/curl.h>)

namespace net {

HttpRequest::HttpRequest(const std::string& url) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::setHeaders(const HttpHeaders& headers) {}

bool HttpRequest::send(HttpResponse& response) {return false;}

void HttpRequest::abort() {}

void HttpRequest::setPostBody(const std::string& body) {}

} // namespace net

#endif
