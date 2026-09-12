// LibreSprite
// Copyright (C) 2023-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// Shared helpers for the delta `Extension`-based script API
//
// The old V8/Duktape engine wrapped doc objects with `getScriptObject(ptr)`
// and resolved `self` via `handle<T>()`. The delta engine instead keeps a
// `std::shared_ptr<void>` per JS wrapper and passes the raw pointer as `self`.
// Doc objects are owned by the document model (not by us), so we wrap them in
// a no-op-deleter shared_ptr: the wrapper "keeps a reference" without taking
// ownership. If the document is destroyed, a stale JS object calling a method
// is UB

#pragma once

#include <memory>

namespace script_api {

  // Stateless marker "sites" for proxy classes whose methods re-resolve the
  // active document on every access (instead of capturing a snapshot).
  struct SpriteSite {};
  struct SelectionSite {};
  struct PixelColorSite {};

  // Wrap a non-owning raw pointer in a shared_ptr with a no-op deleter.
  template<typename T>
  std::shared_ptr<T> wrap(T* p) {
    return std::shared_ptr<T>(p, [](T*) {});
  }

} // namespace script_api
