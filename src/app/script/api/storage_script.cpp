// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/resource_finder.h"
#include "app/script/api/http_internal.h"
#include "app/script/app_scripting.h"
#include "app/script/api/storage_internal.h"
#include "app/script/script_permissions.h"
#include "base/base64.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"

#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

  // Empty domain -> the default storage domain (the current script BASENAME,
  // script::defaultStorageDomain, shared with the entry/intentry widgets).
  std::string normalizeDomain(const std::string& domain) {
    return domain.empty() ? script::defaultStorageDomain() : domain;
  }

  // The on-disk path for a key/domain (user dir, created if missing).
  // Throws base::Exception if key/domain would let the file escape the
  // storage directory.
  std::string storagePath(const std::string& key, const std::string& domain) {
    if (base::has_path_traversal(key) || base::has_path_traversal(domain))
      throw base::Exception("Invalid storage key/domain.");
    app::ResourceFinder rf;
    rf.includeUserDir((domain + "." + key).c_str());
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
      // A --set value wins over the saved value: keep the in-memory one.
      if (script::isCliStorage(key, domain))
        return JSON::Value{true};
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
    // Async: seeds the key with "", performs the request on a worker
    // thread, and on completion stores the body + `<key>_status` and
    // raises `<key>_fetch`. A "POST" header key sets the request body
    // instead of a header.
    addVarArgFunction("__storageFetch") = [](JSON::Array& args) -> JSON::Value {
      if (args.size() < 2)
        return JSON::Value{JSON::Special::Undefined};
      // storage.fetch performs a network request: gate it like fetch.
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kNetFetch)) {
        app::AppScripting::consolePrint(app::ScriptPermissions::denyMessage(app::ScriptPermissions::kNetFetch));
        return JSON::Value{JSON::Special::Undefined};
      }
      auto url = args[0].toString();
      auto key = args[1].toString();
      auto domain = normalizeDomain(args.size() >= 3 ? args[2].toString() : "");
      auto fileName = app::AppScripting::getFileName();
      script::setStorage(JSON::Value{std::string{}}, key, domain);

      std::map<std::string, std::string> headers;
      std::string body;
      bool isPost = false;
      for (std::size_t i = 3; i + 1 < args.size(); i += 2) {
        auto hkey = args[i].toString();
        auto hval = args[i + 1].toString();
        if (hkey == "POST") {
          body = hval;
          isPost = true;
        } else {
          headers[hkey] = hval;
        }
      }

      script::httpFetch(url, isPost ? "POST" : "", body, false, headers, [key, domain, fileName](script::HttpResult&& res) {
        auto status = res.error ? 0 : res.status;
        script::setStorage(JSON::Value{res.error ? std::string{} : std::move(res.body)}, key, domain);
        script::setStorage(JSON::Value{(double)status}, key + "_status", domain);
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
