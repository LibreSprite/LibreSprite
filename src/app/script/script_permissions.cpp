// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/script_permissions.h"

#include "app/resource_finder.h"
#include "base/string.h"
#include "base/trim_string.h"
#include "delta/Interpreter.hpp"
#include "delta/JSON.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

namespace app {

  const char* const ScriptPermissions::kFsRead = "fs.read";
  const char* const ScriptPermissions::kFsWrite = "fs.write";
  const char* const ScriptPermissions::kFetch = "fetch";
  const char* const ScriptPermissions::kAppLaunch = "app.launch";

  namespace {

    bool isKnown(const std::string& name) {
      return name == ScriptPermissions::kFsRead ||
             name == ScriptPermissions::kFsWrite ||
             name == ScriptPermissions::kFetch ||
             name == ScriptPermissions::kAppLaunch;
    }

    // The on-disk path of the grant records (user dir).
    std::string storePath() {
      ResourceFinder rf{false};
      rf.includeUserDir("script_permissions.json");
      return rf.defaultFilename();
    }

    // The in-memory grant records (loaded from the store file on first
    // use; only the main thread touches them).
    std::map<std::string, std::set<std::string>>& records() {
      static std::map<std::string, std::set<std::string>> store;
      static bool loaded = false;
      if (!loaded) {
        loaded = true;
        std::ifstream file{storePath(), std::ios::binary};
        if (!file)
          return store;
        auto source = std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        JSON::Value root;
        try {
          root = JSON::parse(source);
        } catch (const std::exception&) {
          return store; // Corrupt file: start with an empty store
        }
        if (!root.isObject())
          return store;
        for (auto& [path, value] : root.object().map) {
          if (!value.isObject())
            continue;
          auto it = value.object().find("granted");
          if (it == value.object().end() || !it->second.isArray())
            continue;
          std::set<std::string> perms;
          for (auto& entry : it->second.array())
            if (entry.isString() && isKnown(entry.string()))
              perms.insert(entry.string());
          store[path] = std::move(perms);
        }
      }
      return store;
    }

    void saveStore(const std::map<std::string, std::set<std::string>>& store) {
      JSON::Value root;
      root.object();
      for (auto& [path, perms] : store) {
        JSON::Value entry;
        entry.object();
        for (auto& perm : perms)
          entry["granted"].array().push_back(perm);
        root[path] = entry;
      }
      auto path = storePath();
      std::error_code ec;
      std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
      std::ofstream file{path, std::ios::binary | std::ios::trunc};
      if (!file)
        return;
      file << root.toJSON(1);
    }

    // The token string for kUnrestrictedToken.
    const char kUnrestrictedTokenName[] = "<unrestricted>";

    // Interned token strings: std::map keys are stable for the process
    // lifetime, so the c_str() pointer can serve as the token.
    std::map<std::string, std::string>& tokenInterns() {
      static std::map<std::string, std::string> interns;
      return interns;
    }

  } // anonymous namespace

  const void* ScriptPermissions::kUnrestrictedToken = kUnrestrictedTokenName;

  const void* ScriptPermissions::tokenFor(const std::string& path) {
    return tokenInterns().try_emplace(path, path).first->second.c_str();
  }

  bool ScriptPermissions::isKnownPermission(const std::string& name) {
    return isKnown(name);
  }

  std::set<std::string> ScriptPermissions::parseRequested(const std::string& path) {
    std::set<std::string> result;
    std::ifstream file{path};
    if (!file)
      return result;
    std::string line;
    while (std::getline(file, line)) {
      base::trim_string(line, line);
      auto parts = base::split(line, ' ');
      for (auto& part : parts)
        base::trim_string(part, part);
      if (parts.empty() || parts[0].size() < 3 || parts[0].substr(0, 3) != "///")
        break;
      base::trim_string(parts[0].substr(3), parts[0]);
      if (parts[0].empty()) {
        parts.erase(parts.begin());
        if (parts.empty())
          break;
      }
      auto key = base::string_to_lower(parts[0]);
      parts.erase(parts.begin());
      if (key != "permission")
        continue;
      for (auto& perm : parts)
        if (isKnown(perm))
          result.insert(perm);
    }
    return result;
  }

  std::set<std::string> ScriptPermissions::granted(const std::string& path) {
    auto& store = records();
    auto it = store.find(path);
    return it == store.end() ? std::set<std::string>{} : it->second;
  }

  void ScriptPermissions::setGranted(const std::string& path, const std::set<std::string>& perms) {
    records()[path] = perms;
    saveStore(records());
  }

  void ScriptPermissions::removeRecord(const std::string& path) {
    records().erase(path);
    saveStore(records());
  }

  std::map<std::string, std::set<std::string>> ScriptPermissions::allRecords() {
    return records();
  }

  bool ScriptPermissions::check(const std::string& perm) {
    auto* token = Interpreter::currentToken();
    if (!token)
      return false; // No active context: deny
    if (token == kUnrestrictedToken)
      return true;
    return granted(std::string{static_cast<const char*>(token)}).count(perm) > 0;
  }

  std::string ScriptPermissions::denyMessage(const std::string& perm) {
    return "Permission denied: '" + perm + "' permission not granted to this script";
  }

} // namespace app
