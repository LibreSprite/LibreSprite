// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// HTTP extension: ports the fetch API from DeltaAI's HTTP_JS/HTTPBase
// (cpp-httplib based) onto LibreSprite's TaskManager. The blocking
// request runs on a worker thread; the callback is delivered on the
// main thread, where the JS Promise is resolved.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/script/api/http_internal.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class HttpExtension : public Extension {
public:
  HttpExtension() {
    // __fetchCB(url, opts, callback) -> undefined.
    // Async: performs the request on a worker thread and invokes
    // callback on the main thread with a single argument:
    // {status, reason, headers, buffer} or {error: true, reason}.
    addFunction("__fetchCB") = [this](const std::string& url, JSON::ObjectRef opts, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      // Shared ownership of the JS callback (the marshalled Value is
      // destroyed when this call returns).
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      {
        std::lock_guard<std::mutex> guard(callbackMutex);
        pendingCallbacks.push_back(cb);
      }

      std::string method;
      std::string bodyStr;
      bool isBinaryBody{false};
      std::map<std::string, std::string> headers;
      if (opts) {
        if (auto it = opts->find("method"); it != opts->end() && it->second.isString())
          method = it->second.string();
        if (auto it = opts->find("body"); it != opts->end()) {
          if (it->second.isString()) {
            bodyStr = it->second.string();
          } else if (it->second.isByteArray()) {
            auto& arr = it->second.byteArray();
            bodyStr.assign(arr.begin(), arr.end());
            isBinaryBody = true;
          }
        }
        if (auto it = opts->find("headers"); it != opts->end() && it->second.isObject()) {
          for (auto& [key, val] : it->second.object())
            headers[key] = val.string();
        }
      }

      // The JS callback is owned by pendingCallbacks and released on the
      // main thread: the task object owning this lambda is destroyed on a
      // worker thread, and the QuickJS value must not be freed there.
      std::weak_ptr<JSON::Function> cbWeak{cb};
      script::httpFetch(url, method, bodyStr, isBinaryBody, headers, [this, cbWeak](script::HttpResult&& res) {
        JSON::Value rsp;
        rsp.object();
        if (res.error) {
          rsp["error"] = true;
          rsp["reason"] = res.reason;
        } else {
          rsp["status"].number() = res.status;
          rsp["reason"] = res.reason;
          rsp["buffer"].byteArray() = {res.body.begin(), res.body.end()};
          rsp["headers"].object();
          for (auto& [key, val] : res.headers)
            rsp["headers"][key] = val;
        }
        if (auto cb = cbWeak.lock()) {
          JSON::Array args{std::move(rsp)};
          (*cb)(args);
          std::lock_guard<std::mutex> guard(callbackMutex);
          pendingCallbacks.erase(std::remove_if(pendingCallbacks.begin(), pendingCallbacks.end(),
            [&cb](const auto& p) { return p == cb; }),
            pendingCallbacks.end());
        }
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __utf8decode(bytes) -> string. Decodes a byte array as UTF-8: the
    // JSON layer converts the result through JS_NewStringLen, which maps
    // invalid sequences to U+FFFD and astral characters to surrogate
    // pairs (the runtime has no TextDecoder).
    addFunction("__utf8decode") = [](JSON::Value& bytes) -> JSON::Value {
      auto& buf = bytes.byteArray();
      return JSON::Value{std::string(buf.begin(), buf.end())};
    };
  }

  void shutdown() override {
    // Release the JS callbacks of in-flight fetches on the main thread,
    // before the engine is destroyed: the task objects that own them are
    // destroyed later (possibly on worker threads).
    std::lock_guard<std::mutex> guard(callbackMutex);
    pendingCallbacks.clear();
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return R"(
globalThis.fetch = function(url, opts = {}) {
return new Promise((resolve, reject) => {
    __fetchCB(url, opts, (rsp) => {
        try {
            if (rsp.error) {
                reject(new Error(rsp.reason || "Request failed"));
                return;
            }
            const ok = rsp.status >= 200 && rsp.status < 300;
            let bodyUsed = false;
            const response = {
                ok: ok,
                status: rsp.status,
                statusText: rsp.reason || "",
                headers: new Headers(rsp.headers || {}),
                url: url,
                text: () => {
                    if (bodyUsed) {
                        return Promise.reject(new TypeError("Body has already been consumed"));
                    }
                    bodyUsed = true;
                    return Promise.resolve(__utf8decode(rsp.buffer));
                },
                json: () => {
                    if (bodyUsed) {
                        return Promise.reject(new TypeError("Body has already been consumed"));
                    }
                    bodyUsed = true;
                    try {
                        return Promise.resolve(JSON.parse(__utf8decode(rsp.buffer)));
                    } catch (e) {
                        return Promise.reject(new Error("Failed to parse JSON: " + e.message));
                    }
                },
                arrayBuffer: () => {
                    if (bodyUsed) {
                        return Promise.reject(new TypeError("Body has already been consumed"));
                    }
                    bodyUsed = true;
                    return Promise.resolve(rsp.buffer);
                },
            };
            resolve(response);
        } catch (ex) {
            reject(ex);
        }
    });
});
}
globalThis.Headers = class Headers {
    constructor(init = {}) {
        this._map = new Map();
        if (init instanceof Headers) {
            init._map.forEach((v, k) => this._map.set(k, v));
        } else if (typeof init === "object" && init !== null) {
            Object.entries(init).forEach(([k, v]) => this._map.set(k.toLowerCase(), v));
        }
    }
    get(name) { return this._map.get(name.toLowerCase()); }
    set(name, value) { this._map.set(name.toLowerCase(), value); }
    has(name) { return this._map.has(name.toLowerCase()); }
    delete(name) { return this._map.delete(name.toLowerCase()); }
    forEach(cb) { this._map.forEach((v, k) => cb(v, k)); }
};
)";
  }

private:
  // JS callbacks of in-flight fetches, released on the main thread (see
  // shutdown() and the __fetchCB consumer).
  std::mutex callbackMutex;
  std::vector<std::shared_ptr<JSON::Function>> pendingCallbacks;
};

static di::provide<Extension, HttpExtension> httpExt{"http"};
