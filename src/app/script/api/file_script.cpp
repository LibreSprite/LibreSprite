// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// The `file` object: file format registration from scripts and the
// codec reply API. file.registerFormat({name, extensions, load, save})
// registers a ScriptFileFormat in the FileFormatsManager, bound to the
// calling script's canonical path; load/save of a file with the format's
// extension raises a format_load/format_save event (with a request id)
// to the script, which replies through file.onLoad/onSave (or
// file.onLoadError/onSaveError, and file.resetWatchdog for chunked work).

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include "app/file/file_formats_manager.h"
#include "app/file/script_codec_requests.h"
#include "app/script/app_scripting.h"

#include <string>

class FileObject {
public:
};

class FileExtension : public Extension {
public:
  FileExtension() {
    auto& cls = addClass<void, FileObject>("File");
    cls.setConstructor() = []() -> std::shared_ptr<FileObject> {
      return std::make_shared<FileObject>();
    };

    // registerFormat({name, extensions, load, save}):
    // - name: display name (e.g. "PPM/PGM")
    // - extensions: comma-separated extensions (e.g. "ppm,pgm")
    // - load/save: booleans
    // Re-calling for the same script path replaces the prior registration.
    cls.addMethod("registerFormat") = [](FileObject&, JSON::Value& spec) -> JSON::Value {
      const char* usage = "file.registerFormat: expected {name, extensions, load, save}";
      if (!spec.isObject()) {
        app::AppScripting::consolePrint(usage);
        return JSON::Value{JSON::Special::Null};
      }
      auto& obj = spec.object();
      auto nameIt = obj.find("name");
      auto extIt = obj.find("extensions");
      if (nameIt == obj.end() || !nameIt->second.isString() ||
          extIt == obj.end() || !extIt->second.isString() ||
          nameIt->second.string().empty() || extIt->second.string().empty()) {
        app::AppScripting::consolePrint(usage);
        return JSON::Value{JSON::Special::Null};
      }
      app::FileFormatsManager::instance()->registerScriptFormat(
        app::AppScripting::getFileName(),
        nameIt->second.string(),
        extIt->second.string(),
        obj.contains("load") && obj["load"].isBoolean() && obj["load"].boolean(),
        obj.contains("save") && obj["save"].isBoolean() && obj["save"].boolean());
      return {};
    };

    // Codec reply API (the format_load/format_save dispatch bridge):
    // settles the in-flight request the id refers to (created by
    // ScriptFileFormat). Unknown or completed ids are a no-op.
    cls.addMethod("onLoad") = [](FileObject&, double id, JSON::Value& result) -> JSON::Value {
      if (!app::ScriptCodecRequests::resolve((int)id, std::move(result)))
        app::AppScripting::consolePrint(
          "file.onLoad: unknown or completed request id " + std::to_string((int)id));
      return {};
    };

    cls.addMethod("onSave") = [](FileObject&, double id, JSON::Value& bytes) -> JSON::Value {
      if (!app::ScriptCodecRequests::resolve((int)id, std::move(bytes)))
        app::AppScripting::consolePrint(
          "file.onSave: unknown or completed request id " + std::to_string((int)id));
      return {};
    };

    cls.addMethod("onLoadError") = [](FileObject&, double id, const std::string& message) -> JSON::Value {
      if (!app::ScriptCodecRequests::fail((int)id, message))
        app::AppScripting::consolePrint(
          "file.onLoadError: unknown or completed request id " + std::to_string((int)id));
      return {};
    };

    cls.addMethod("onSaveError") = [](FileObject&, double id, const std::string& message) -> JSON::Value {
      if (!app::ScriptCodecRequests::fail((int)id, message))
        app::AppScripting::consolePrint(
          "file.onSaveError: unknown or completed request id " + std::to_string((int)id));
      return {};
    };

    // Re-arms the 2s watchdog window for the in-flight request: a codec
    // that works in chunks calls this (and awaits app.yield()) between
    // chunks so the loop-visible time never exceeds the window.
    cls.addMethod("resetWatchdog") = [](FileObject&, double id) -> JSON::Value {
      if (!app::ScriptCodecRequests::reset((int)id))
        app::AppScripting::consolePrint(
          "file.resetWatchdog: unknown or completed request id " + std::to_string((int)id));
      return {};
    };
  }

  std::string init(const std::string&, JSON::Value&) override {
    return "globalThis.file = new File();";
  }
};

static di::provide<Extension, FileExtension> fileExt{"file"};
