// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/script/api/storage_internal.h"

#include "app/script/app_scripting.h"
#include "base/path.h"

#include <unordered_map>
#include <unordered_set>

namespace {
  std::unordered_map<std::string, std::unordered_map<std::string, JSON::Value>> g_storage;
  // Keys seeded from the command line (--set key=value), by domain:
  // storage.load() must not overwrite them with the saved on-disk value
  // until the script assigns or removes the key itself.
  std::unordered_map<std::string, std::unordered_set<std::string>> g_cliStorage;

  void dropCliMark(const std::string& key, const std::string& domain) {
    auto domainIt = g_cliStorage.find(domain);
    if (domainIt == g_cliStorage.end())
      return;
    domainIt->second.erase(key);
    if (domainIt->second.empty())
      g_cliStorage.erase(domainIt);
  }
}

namespace script {
  void setStorage(const JSON::Value& value, const std::string& key, const std::string& domain) {
    g_storage[domain][key] = value;
    // The script (or a widget) took ownership of the key: a --set value
    // no longer protects it from storage.load().
    dropCliMark(key, domain);
  }

  void setCliStorage(const JSON::Value& value, const std::string& key, const std::string& domain) {
    g_storage[domain][key] = value;
    g_cliStorage[domain].insert(key);
  }

  bool isCliStorage(const std::string& key, const std::string& domain) {
    auto domainIt = g_cliStorage.find(domain);
    return domainIt != g_cliStorage.end() && domainIt->second.count(key) > 0;
  }

  JSON::Value getStorage(const std::string& key, const std::string& domain) {
    auto domainIt = g_storage.find(domain);
    if (domainIt == g_storage.end())
      return JSON::Value{JSON::Special::Null};
    auto entryIt = domainIt->second.find(key);
    if (entryIt == domainIt->second.end())
      return JSON::Value{JSON::Special::Null};
    return entryIt->second;
  }

  void removeStorage(const std::string& key, const std::string& domain) {
    auto domainIt = g_storage.find(domain);
    if (domainIt != g_storage.end()) {
      domainIt->second.erase(key);
      if (domainIt->second.empty())
        g_storage.erase(domainIt);
    }
    dropCliMark(key, domain);
  }

  std::string defaultStorageDomain() {
    return base::get_file_name(app::AppScripting::getFileName());
  }
}
