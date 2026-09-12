// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/script/api/storage_internal.h"

#include <unordered_map>

namespace {
  std::unordered_map<std::string, std::unordered_map<std::string, JSON::Value>> g_storage;
}

namespace script {
  void setStorage(const JSON::Value& value, const std::string& key, const std::string& domain) {
    g_storage[domain][key] = value;
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
    if (domainIt == g_storage.end())
      return;
    domainIt->second.erase(key);
    if (domainIt->second.empty())
      g_storage.erase(domainIt);
  }
}
