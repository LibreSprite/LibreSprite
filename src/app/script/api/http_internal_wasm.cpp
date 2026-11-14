// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#if defined(__EMSCRIPTEN__)

#include "app/script/api/http_internal.h"

#include "delta/StringUtils.hpp"

#include <emscripten/emscripten.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

  // The JS fetch handler writes the response as a blob (little-endian):
  // int32 status, int32 error, length-prefixed UTF-8 reason, int32
  // headerCount, length-prefixed header name/value pairs, then the
  // length-prefixed body bytes.
  class BlobReader {
    const std::uint8_t* p;
    const std::uint8_t* end;
    bool m_ok = true;

  public:
    BlobReader(const std::uint8_t* data, std::size_t size) : p{data}, end{data + size} {}

    bool ok() const { return m_ok; }

    std::uint32_t u32() {
      if (p + 4 > end) {
        m_ok = false;
        return 0;
      }
      std::uint32_t v;
      std::memcpy(&v, p, 4);
      p += 4;
      return v;
    }

    std::vector<std::uint8_t> bytes() {
      std::uint32_t n = u32();
      if (p + n > end) {
        m_ok = false;
        return {};
      }
      std::vector<std::uint8_t> b(p, p + n);
      p += n;
      return b;
    }
  };

  // Shared with the JS fetch handler (main thread). Heap-allocated so the
  // handler can still write it if the request completes after the worker
  // gave up (abort at shutdown).
  struct FetchArgs {
    const char* url;
    const char* method;
    const char* headers;
    const char* body;
    std::uintptr_t bodySize;
    int binaryBody;
    std::uintptr_t response;
    std::uintptr_t responseSize;
    std::atomic<std::uint32_t> done{0};
  };

  // Performs the request with the browser fetch() (blocking; runs on a
  // TaskManager worker while the JS promise resolves on the main thread).
  script::HttpResult performRequest(const std::string& url, const std::string& method,
                                    const std::string& body, bool isBinaryBody,
                                    const std::map<std::string, std::string>& headers,
                                    const std::atomic_bool& aborted) {
    script::HttpResult result;

    // fetch() rejects a body with GET/HEAD (the native path ignores it)
    std::string reqBody = body;
    if (method == "GET" || method == "HEAD")
      reqBody.clear();

    // Build headers: user headers + default Content-Type (user overrides
    // default). User-Agent is skipped: the browser forbids overriding it.
    std::string headerStr;
    bool hasContentType = false;
    for (const auto& [key, val] : headers) {
      if (su::toLower(key) == "content-type")
        hasContentType = true;
      headerStr += key + std::string(": ") + val + "\n";
    }
    if (!reqBody.empty() && !hasContentType)
      headerStr += "Content-Type: " + std::string(isBinaryBody ? "application/octet-stream" : "application/json") + "\n";

    auto args = std::make_unique<FetchArgs>();
    args->url = url.c_str();
    args->method = method.c_str();
    args->headers = headerStr.c_str();
    args->body = reqBody.c_str();
    args->bodySize = reqBody.size();
    args->binaryBody = isBinaryBody ? 1 : 0;

    MAIN_THREAD_EM_ASM({
      // Bare HEAP* references are rewritten by emscripten to
      // (growMemViews(), HEAP*) so they stay valid after memory growth.
      const index = 'url,method,headers,body,bodySize,binaryBody,response,responseSize,done'.split(',');
      const enc = new TextEncoder();

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
      const method = UTF8ToString(get('method'));
      const headers = UTF8ToString(get('headers'));
      const bodySize = get('bodySize');

      const parsedHeaders = {};
      headers.split('\n').forEach(line => {
        const i = line.indexOf(':');
        if (i > 0)
          parsedHeaders[line.substring(0, i).trim()] = line.substring(i + 1).trim();
      });

      // Copy the body out of the heap: the request may be sent after the
      // worker moves on, and the heap can grow in the meantime.
      let body;
      if (bodySize > 0) {
        body = get('binaryBody') ? HEAP8.slice(get('body'), get('body') + bodySize)
                                 : UTF8ToString(get('body'), bodySize);
      }

      function finish(status, error, reason, headerPairs, bodyBytes) {
        const reasonB = enc.encode(reason || "");
        const parts = [];
        for (const [n, v] of headerPairs)
          parts.push(enc.encode(n), enc.encode(v));
        // 5 fixed int32 fields (status, error, reason length, header
        // count, body length) + the variable-length fields below.
        let total = 20 + reasonB.length + bodyBytes.length;
        for (const b of parts)
          total += 4 + b.length;
        const ptr = _malloc(total);
        const heap = HEAPU8;
        const view = new DataView(heap.buffer, ptr, total);
        let off = 0;
        // view is based at ptr, the heap views are based at 0
        const w32 = v => { view.setInt32(off, v, true); off += 4; };
        const wb = b => { w32(b.length); heap.set(b, ptr + off); off += b.length; };
        w32(status);
        w32(error);
        wb(reasonB);
        w32(headerPairs.length);
        for (let i = 0; i < parts.length; i += 2) {
          wb(parts[i]);
          wb(parts[i + 1]);
        }
        wb(bodyBytes);
        set('response', ptr);
        set('responseSize', total);
        // Release the flag after the blob: the worker loads it with acquire
        Atomics.store(HEAPU32, ($0 >> 2) + index.indexOf('done'), 1);
      }

      try {
        fetch(url, {
          method,
          mode: 'cors',
          headers: parsedHeaders,
          body
        })
        .then(response => {
          const headerPairs = [];
          response.headers.forEach((value, name) => headerPairs.push([name, value]));
          return response.arrayBuffer().then(buffer =>
            finish(response.status, 0, response.statusText, headerPairs, new Uint8Array(buffer)));
        })
        .catch(ex => finish(0, 1, String(ex && ex.message ? ex.message : ex), [], new Uint8Array(0)));
      } catch (ex) {
        finish(0, 1, String(ex && ex.message ? ex.message : ex), [], new Uint8Array(0));
      }
    }, args.get(), sizeof(std::uintptr_t));

    // Wait for the JS handler or for an abort (TaskManager shutdown: the
    // main thread then joins this thread, so the event loop cannot resolve
    // the fetch anymore).
    while (args->done.load(std::memory_order_acquire) == 0 &&
           !aborted.load(std::memory_order_acquire))
      std::this_thread::sleep_for(std::chrono::milliseconds(30));

    if (args->done.load(std::memory_order_acquire) == 0) {
      // Aborted while waiting: the JS handler may still write the blob, so
      // leak the struct for the process to reclaim (the handler keeps a
      // raw pointer to it).
      args.release();
      result.error = true;
      result.reason = "Request aborted";
      return result;
    }

    // The handler wrote the blob before releasing the done flag
    auto* blob = reinterpret_cast<void*>(args->response);
    const auto* data = static_cast<const std::uint8_t*>(blob);
    const std::size_t size = args->responseSize;

    if (!blob) {
      result.error = true;
      result.reason = "Empty response";
      args.reset();
      return result;
    }

    // Parse before freeing: a concurrent _malloc could reuse the blob
    // region while it is still being read.
    BlobReader r{data, size};
    result.status = (int)r.u32();
    result.error = (r.u32() != 0);
    auto reasonB = r.bytes();
    result.reason.assign(reasonB.begin(), reasonB.end());
    const std::uint32_t headerCount = r.u32();
    for (std::uint32_t i = 0; i < headerCount && r.ok(); ++i) {
      auto nameB = r.bytes();
      auto valB = r.bytes();
      result.headers.emplace(std::string(nameB.begin(), nameB.end()),
                             std::string(valB.begin(), valB.end()));
    }
    auto bodyB = r.bytes();
    std::free(blob);
    args.reset();
    if (!r.ok()) {
      result = script::HttpResult{};
      result.error = true;
      result.reason = "Malformed response";
      return result;
    }
    result.body.assign(bodyB.begin(), bodyB.end());
    return result;
  }

} // namespace

namespace script {

  app::TaskHandle httpFetch(const std::string& url, const std::string& methodIn, const std::string& body,
                            bool isBinaryBody, const std::map<std::string, std::string>& headers,
                            std::function<void(HttpResult&&)>&& cb) {
    // fetch() only accepts http(s) URLs (same as the native parseUrl)
    const std::string lowerUrl = su::toLower(url);
    const std::string parseError =
      (lowerUrl.rfind("http://", 0) == 0 || lowerUrl.rfind("https://", 0) == 0) ? "" : "Invalid URL";
    const std::string method = methodIn.empty() ? "GET" : su::toUpper(methodIn);

    auto aborted = std::make_shared<std::atomic_bool>(false);

    return app::TaskManager::instance().addTask<HttpResult>(
      [url, method, body, isBinaryBody, headers, parseError, aborted]{
        if (!parseError.empty()) {
          HttpResult result;
          result.error = true;
          result.reason = parseError;
          return result;
        }
        try {
          return performRequest(url, method, body, isBinaryBody, headers, *aborted);
        } catch (const std::exception& e) {
          HttpResult result;
          result.error = true;
          result.reason = e.what();
          return result;
        }
      },
      std::move(cb),
      [aborted]{ aborted->store(true, std::memory_order_release); });
  }

}

#endif // __EMSCRIPTEN__
