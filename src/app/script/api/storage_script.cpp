// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "doc/color.h"
#include "doc/pixel_format.h"
#include "script/engine.h"
#include "app/script/app_scripting.h"
#include "app/res/http.h"
#include <string>

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
    addMethod("fetch", &StorageScriptObject::fetch);
    makeGlobal("storage");
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

  void fetch(const std::string& url, const std::string& key, const std::string& domain) {
    auto fileName = app::AppScripting::getFileName();
    auto domainKey = domain.empty() ? fileName : domain;
    std::cout << "Fetching " << url << " into " << key << std::endl;
    app::HTTP::get(url, [=](app::HTTP::Result&& result) {
      storage[domainKey][key] = std::move(result.body);
      storage[domainKey][key + "_status"] = result.status;
      app::AppScripting::raiseEvent(fileName, key + "_fetch");
    });
  }
};

static script::ScriptObject::Regular<StorageScriptObject> reg("StorageScriptObject", {"global"});
