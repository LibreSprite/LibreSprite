// Aseprite Network Library
// Aseprite    | Copyright (C) 2001-2016 David Capello
// LibreSprite | Copyright (C) 2026      LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "net/http_request.h"

#include "base/debug.h"
#include "net/http_response.h"

#if __has_include(<curl/curl.h>)

#include <curl/curl.h>

namespace net {

class HttpRequestImpl {
public:
  HttpRequestImpl(const std::string& url)
    : m_curl(curl_easy_init())
    , m_headerlist(nullptr)
    , m_response(nullptr) {
    // UNSAFE
    // TLS verification must stay on unconditionally - being it disabled
    // on Android lets anyone on the network path intercept or tamper
    // with every request (including script-initiated fetches).
    // This was supposedly disabled due to different/incompatible SSL library
    // ToDo: confirm the problem with SSL library and provide safe solutions
    // If Android needs a CA bundle, supply one via CURLOPT_CAINFO rather
    // than skipping verification.
#ifdef ANDROID
    curl_easy_setopt(m_curl,  CURLOPT_SSL_VERIFYPEER, 0);
#endif
    curl_easy_setopt(m_curl, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpRequestImpl::writeBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    // CURLOPT_PROTOCOLS_STR needs libcurl >= 7.85.0; fall back to the older
    // (but equivalent) bitmask option on earlier versions so this still
    // builds against whatever libcurl a given platform ships.
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
  }

  ~HttpRequestImpl() {
    if (m_headerlist)
      curl_slist_free_all(m_headerlist);

    curl_easy_cleanup(m_curl);
  }

  void setPostBody(const std::string& body) {
    m_body = body;
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_body.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)m_body.size());
  }

  void setHeaders(const HttpHeaders& headers) {
    if (m_headerlist) {
      curl_slist_free_all(m_headerlist);
      m_headerlist = NULL;
    }

    std::string tmp;
    for (auto& entry : headers) {
      tmp = entry.first;
      tmp += ": ";
      tmp += entry.second;
      m_headerlist = curl_slist_append(m_headerlist, tmp.c_str());
    }

    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headerlist);
  }

  bool send(HttpResponse& response) {
    m_response = &response;
    int res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
      return false;

    long code;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
    m_response->setStatus(code);
    return true;
  }

  void abort() {
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, 1);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1);
  }

private:
  std::size_t writeBody(char* ptr, std::size_t bytes) {
    ASSERT(m_response != NULL);
    m_response->write(ptr, bytes);
    return bytes;
  }

  static std::size_t writeBodyCallback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    HttpRequestImpl* req = reinterpret_cast<HttpRequestImpl*>(userdata);
    return req->writeBody(ptr, size*nmemb);
  }

  CURL* m_curl;
  curl_slist* m_headerlist;
  HttpResponse* m_response;
  std::string m_body;
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
