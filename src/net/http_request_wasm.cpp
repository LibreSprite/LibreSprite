// LibreSprite
// Copyright (c) 2024 LibreSprite Contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__EMSCRIPTEN__)
#include "net/http_request.h"

#include "base/debug.h"
#include "net/http_response.h"
#include "net/http_response.h"

#include <emscripten/emscripten.h>
#include <thread>
#include <chrono>

namespace net {

class HttpRequestImpl {
  std::string m_url;
  std::string m_body;
  std::string m_headers;
  bool m_isPOST{};
  bool cancelled{};

  public:
  HttpRequestImpl(const std::string& url) : m_url{url} {}

  void setPostBody(const std::string& body) {
    m_isPOST = true;
    m_body = body;
  }

  void setHeaders(const HttpHeaders& headers) {
    std::string tmp;
    for (auto& entry : headers) {
      tmp = entry.first;
      tmp += ": ";
      tmp += entry.second;
      if (!m_headers.empty()) m_headers += "\n";
      m_headers += tmp;
    }
  }

  bool send(HttpResponse& response) {
    struct {
      const char* url;
      const char* body;
      uintptr_t bodySize;
      const char* headers;
      char* response;
      uintptr_t responseSize;
      uintptr_t statusCode;
    } args {
      m_url.c_str(), // url
      m_body.c_str(), // body
      m_body.size(), // bodySize
      m_headers.c_str(), // headers
      nullptr, // response
      0, // responseSize
      ~uintptr_t{}, // statusCode
    };

    MAIN_THREAD_EM_ASM({
	const index = 'url,body,bodySize,headers,response,responseSize,statusCode,callbackArg,callback'.split(',');
	const HEAP32 = GROWABLE_HEAP_U32();
	const HEAP8 = GROWABLE_HEAP_U8();

	function get(name) {
	  if ($1 == 4)
	    return HEAP32[($0 >> 2) + index.indexOf(name)];
	  else
	    return HEAP64[($0 >> 3) + index.indexOf(name)];
	}
	function set(name, value) {
	  if ($1 == 4)
	    HEAP32[($0 >> 2) + index.indexOf(name)] = value;
	  else
	    HEAP64[($0 >> 3) + index.indexOf(name)] = value;
	}

	const url = UTF8ToString(get('url'));

	const isPOST = $2;
	const body = isPOST ? UTF8ToString(get('body')) : undefined;

	const headers = UTF8ToString(get('headers'));
	const parsedHeaders = {};
	headers.split('\n').forEach(line => {
	    const arr = line.split(':');
	    if (arr.length == 2)
	      parsedHeaders[arr[0].trim()] = arr[1].trim();
	  });

	fetch(url, {
	  method: isPOST ? 'POST' : 'GET',
	  mode: 'cors',
	  headers: parsedHeaders,
	  body
	  })
	  .then(response => response.arrayBuffer())
	  .then(buffer => {
	      const arr = new Uint8Array(buffer);
	      const raw = _malloc(arr.length);
	      for (let i = 0; i < arr.length; ++i)
		HEAP8[raw + i] = arr[i];
	      set('response', raw);
	      set('responseSize', arr.length);
	      set('statusCode', 200);
	    })
	  .catch(ex => {
	      set('statusCode', 400);
	    });
      }, &args, sizeof(uintptr_t), m_isPOST);

    while (args.statusCode == ~uintptr_t{}) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(30ms);
    }

    response.setStatus(args.statusCode);
    if (!args.response)
      return false;
    response.write(args.response, args.responseSize);
    free(args.response);
    return true;
  }

  void abort() {
    cancelled = true;
  }
};

HttpRequest::HttpRequest(const std::string& url) : m_impl{new HttpRequestImpl(url)} {}
HttpRequest::~HttpRequest() {}

void HttpRequest::setHeaders(const HttpHeaders& headers)
{
  m_impl->setHeaders(headers);
}

bool HttpRequest::send(HttpResponse& response)
{
  return m_impl->send(response);
}

void HttpRequest::abort()
{
  m_impl->abort();
}

void HttpRequest::setPostBody(const std::string& body)
{
  m_impl->setPostBody(body);
}

} // namespace net

#endif
