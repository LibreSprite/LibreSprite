// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "doc/document.h"

#include <memory>
#include <stdexcept>

// `Document` wraps a `doc::Document`. It is not constructible from JS (the
// constructor throws); instances are produced by the app-level API
// (`app.activeDocument`, `app.open`).
//
// `document.sprite` returns the `Sprite` proxy (active-document semantics,
// matching the `sprite` global) rather than a per-document snapshot

class DocumentExtension : public Extension {
public:
  DocumentExtension() {
    auto& clazz = addClass<void, doc::Document>("Document");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Document cannot be constructed directly"};
    };

    clazz.addGetter("sprite") = [](doc::Document&) -> JSON::Value {
      return JSON::makeNative(std::make_shared<script_api::SpriteSite>());
    };

    clazz.addMethod("close") = [](doc::Document& doc) -> JSON::Value {
      doc.close();
      return true;
    };
  }
};

static di::provide<Extension, DocumentExtension> x{"document"};
