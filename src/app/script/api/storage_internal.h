// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <string>
#include "delta/JSON.hpp"

namespace script {
  // In-memory storage map shared by the `storage` global (Phase 3 wave 4) and
  // the entry/intentry widgets (wave 3.2), keyed by domain then key. Values are
  // JSON::Value (script::Value is a typedef for it). Pulled forward from
  // storage_script.cpp so the widgets can persist their values before the
  // storage global itself lands; the wave-4 storage_script.cpp will be updated
  // to use this same map.
  void setStorage(const JSON::Value& value, const std::string& key, const std::string& domain);
  JSON::Value getStorage(const std::string& key, const std::string& domain);
  // Remove a key from the domain's map (no-op if absent). Used by storage.unload.
  void removeStorage(const std::string& key, const std::string& domain);
}
