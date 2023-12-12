// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/res/http.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "base/base64.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/color.h"
#include "doc/pixel_format.h"
#include "net/http_request.h"
#include "script/engine.h"
#include "script/value.h"
#include <cstdio>
#include <string>
#include <optional>
#include <vector>

namespace {

  std::unordered_map<std::string, std::unordered_map<std::string, script::Value>> storage;

};

namespace script {
  void setStorage(const script::Value& value, const std::string& key, const std::string& domain) {
    storage[domain][key] = value;
  }
}

class StorageScriptObject : public script::ScriptObject {
public:
  StorageScriptObject() {
    addMethod("get", &StorageScriptObject::get);
    addMethod("set", &StorageScriptObject::set);
    addMethod("save", &StorageScriptObject::save);
    addMethod("load", &StorageScriptObject::load);
    addMethod("fetch", &StorageScriptObject::fetch);
    addMethod("decodeBase64", &StorageScriptObject::decodeBase64);
    makeGlobal("storage");
  }

  std::string path(const std::string& key, const std::string& domain) {
        app::ResourceFinder rf;
        rf.includeUserDir(((domain.empty() ? app::AppScripting::getFileName() : domain) + "." + key).c_str());
        return rf.getFirstOrCreateDefault();
  }

  script::Value get(const std::string& key, const std::string& domain) {
    auto domainIt = storage.find(domain.empty() ? app::AppScripting::getFileName() : domain);
    if (domainIt == storage.end()) return {};
    auto entryIt = domainIt->second.find(key);
    if (entryIt == domainIt->second.end()) return {};
    return entryIt->second;
  }

  void set(const script::Value& value, const std::string& key, const std::string& domain) {
    storage[domain.empty() ? app::AppScripting::getFileName() : domain][key] = value;
  }

  bool load(const std::string& key, const std::string& domain) {
    try {
      auto path = this->path(key, domain);
      base::FileHandle handle(base::open_file_with_exception(path, "rb"));
      fseek(handle.get(), 0, SEEK_END);
      auto size = ftell(handle.get());
      fseek(handle.get(), 0, SEEK_SET);
      std::vector<unsigned char> data;
      data.resize(size);
      fread(data.data(), size, 1, handle.get());
      set(std::string{data.begin(), data.end()}, key, domain);
    } catch(...) {
      return false;
    }
    return true;
  }

  std::string save(const std::string& key, const std::string& domain) {
    auto domainIt = storage.find(domain.empty() ? app::AppScripting::getFileName() : domain);
    if (domainIt == storage.end()) return "";
    auto entryIt = domainIt->second.find(key);
    if (entryIt == domainIt->second.end()) return "";
    try {
      auto path = this->path(key, domain);
      base::FileHandle handle(base::open_file_with_exception(path, "wb"));
      auto str = entryIt->second.str();
      fwrite(str.c_str(), str.size(), 1, handle.get());
      return path;
    } catch(...) {
      return "";
    }
    return "";
  }

  bool decodeBase64(const std::string& key, const std::string& domain) {
    auto domainIt = storage.find(domain.empty() ? app::AppScripting::getFileName() : domain);
    if (domainIt == storage.end()) return false;
    auto entryIt = domainIt->second.find(key);
    if (entryIt == domainIt->second.end()) return false;
    std::vector<unsigned char> buffer;
    base::decode_base64(entryIt->second.str(), buffer);
    if (buffer.empty())
      return false;
    set(std::string{buffer.begin(), buffer.end()}, key, domain);
    return true;
  }

  void fetch(const std::string& url, const std::string& key, const std::string& domain) {
    auto fileName = app::AppScripting::getFileName();
    auto domainKey = domain.empty() ? fileName : domain;
    storage[domainKey][key] = std::string{};
    std::cout << "Fetching " << url << " into " << key << std::endl;

    auto& args = script::Function::varArgs();
    std::string body;
    std::string* post{};
    std::size_t argc = args.size();

    net::HttpHeaders headers;
    for (std::size_t i = 3; i + 1 < argc; i += 2) {
      auto key = args[i].str();
      auto val = args[i + 1].str();;
      if (key == "POST"){
        body = val;
        post = &body;
      } else {
        headers[key] = val;
      }
    }

    app::HTTP::fetch(url, post, headers, [=](app::HTTP::Result&& result) {
      storage[domainKey][key] = std::move(result.body);
      storage[domainKey][key + "_status"] = result.status;
      app::AppScripting::raiseEvent(fileName, key + "_fetch");
    });
  }
};

static script::ScriptObject::Regular<StorageScriptObject> reg("StorageScriptObject", {"global"});
