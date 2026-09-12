// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/res/http.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "app/script/api/storage_internal.h"
#include "base/base64.h"
#include "base/file_handle.h"
#include "base/fs.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

  // Empty domain -> the current script file name (matches the old behavior +
  // the entry/intentry widgets, which persist under the file name).
  std::string normalizeDomain(const std::string& domain) {
    return domain.empty() ? app::AppScripting::getFileName() : domain;
  }

  // The on-disk path for a key/domain (user dir, created if missing).
  std::string storagePath(const std::string& key, const std::string& domain) {
    app::ResourceFinder rf;
    rf.includeUserDir((normalizeDomain(domain) + "." + key).c_str());
    return rf.getFirstOrCreateDefault();
  }

} // namespace

class StorageExtension : public Extension {
public:
  StorageExtension() {
    // storage.get(key, domain) -> the stored value (null if absent).
    addFunction("__storageGet") = [](const std::string& keyV, const std::string& domainV) -> JSON::Value {
      return script::getStorage(keyV, normalizeDomain(domainV));
    };

    // storage.set(value, key, domain) -> undefined.
    addFunction("__storageSet") = [](JSON::Value& value, const std::string& keyV, const std::string& domainV) -> JSON::Value {
      script::setStorage(value, keyV, normalizeDomain(domainV));
      return JSON::Value{JSON::Special::Undefined};
    };

    // storage.unload(key, domain) -> undefined (removes the key).
    addFunction("__storageUnload") = [](const std::string& keyV, const std::string& domainV) -> JSON::Value {
      script::removeStorage(keyV, normalizeDomain(domainV));
      return JSON::Value{JSON::Special::Undefined};
    };

    // storage.save(key, domain) -> the file path ("" if the key is absent).
    addFunction("__storageSave") = [](const std::string& key, const std::string& domainRaw) -> JSON::Value {
      auto domain = normalizeDomain(domainRaw);
      auto value = script::getStorage(key, domain);
      if (value.isNull() || value.isUndefined())
        return JSON::Value{std::string{}};
      try {
        auto path = storagePath(key, domain);
        base::FileHandle handle(base::open_file_with_exception(path, "wb"));
        auto str = value.toString();
        fwrite(str.c_str(), str.size(), 1, handle.get());
        return JSON::Value{path};
      } catch (...) {
        return JSON::Value{std::string{}};
      }
    };

    // storage.load(key, domain) -> true if the file was read + stored.
    addFunction("__storageLoad") = [](const std::string& key, const std::string& domainV) -> JSON::Value {
      auto domain = normalizeDomain(domainV);
      try {
        auto path = storagePath(key, domain);
        base::FileHandle handle(base::open_file_with_exception(path, "rb"));
        fseek(handle.get(), 0, SEEK_END);
        auto size = ftell(handle.get());
        fseek(handle.get(), 0, SEEK_SET);
        std::vector<unsigned char> data;
        data.resize(size);
        fread(data.data(), size, 1, handle.get());
        script::setStorage(JSON::Value{std::string{data.begin(), data.end()}}, key, domain);
      } catch (...) {
        return JSON::Value{false};
      }
      return JSON::Value{true};
    };

    // storage.decodeBase64(key, domain) -> true if the stored value was
    // base64-decoded in place.
    addFunction("__storageDecodeBase64") = [](const std::string& key, const std::string& domainV) -> JSON::Value {
      auto domain = normalizeDomain(domainV);
      auto value = script::getStorage(key, domain);
      if (value.isNull() || value.isUndefined())
        return JSON::Value{false};
      std::vector<unsigned char> buffer;
      base::decode_base64(value.toString(), buffer);
      if (buffer.empty())
        return JSON::Value{false};
      script::setStorage(JSON::Value{std::string{buffer.begin(), buffer.end()}}, key, domain);
      return JSON::Value{true};
    };

    // storage.fetch(url, key, [domain, header, value, ...]) -> undefined.
    // Async: seeds the key with "", performs app::HTTP::fetch, and on
    // completion stores the body + `<key>_status` and raises `<key>_fetch`.
    // A "POST" header key sets the request body instead of a header.
    addVarArgFunction("__storageFetch") = [](JSON::Array& args) -> JSON::Value {
      if (args.size() < 2)
        return JSON::Value{JSON::Special::Undefined};
      auto url = args[0].toString();
      auto key = args[1].toString();
      auto domain = normalizeDomain(args.size() >= 3 ? args[2].toString() : "");
      auto fileName = app::AppScripting::getFileName();
      script::setStorage(JSON::Value{std::string{}}, key, domain);

      std::unordered_map<std::string, std::string> headers;
      std::string body;
      std::string* post = nullptr;
      for (std::size_t i = 3; i + 1 < args.size(); i += 2) {
        auto hkey = args[i].toString();
        auto hval = args[i + 1].toString();
        if (hkey == "POST") {
          body = hval;
          post = &body;
        } else {
          headers[hkey] = hval;
        }
      }

      app::HTTP::fetch(url, post, headers, [key, domain, fileName](app::HTTP::Result&& result) {
        script::setStorage(JSON::Value{std::move(result.body)}, key, domain);
        script::setStorage(JSON::Value{(double)result.status}, key + "_status", domain);
        JSON::Value event;
        event.push_back(key + "_fetch");
        app::AppScripting::raiseEvent(fileName, event);
      });
      return JSON::Value{JSON::Special::Undefined};
    };
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return "globalThis.storage = { "
           "get: __storageGet, "
           "set: __storageSet, "
           "unload: __storageUnload, "
           "save: __storageSave, "
           "load: __storageLoad, "
           "fetch: __storageFetch, "
           "decodeBase64: __storageDecodeBase64 "
           "};";
  }
};

static di::provide<Extension, StorageExtension> storageExt{"storage"};
