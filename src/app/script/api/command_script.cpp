// LibreSprite
// Copyright (C) 2024-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// The `command` object: run app commands by name. Holds a persistent Params
// bag (setParameter/clearParameters) plus one method per registered command
// (named by the command id).

#include "delta/Extension.hpp"
#include "di.hpp"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ui_context.h"
#include "app/script/api/script_api_common.h"

class CommandObject {
public:
  app::Params params;
};

class CommandExtension : public Extension {
public:
  CommandExtension() {
    auto& cls = addClass<void, CommandObject>("Command");
    cls.setConstructor() = []() -> std::shared_ptr<CommandObject> {
      return std::make_shared<CommandObject>();
    };

    // setParameter / clearParameters mutate the persistent bag and return
    // `this` (the same native object) so calls can be chained.
    cls.addMethod("setParameter") = [](CommandObject& self, const std::string& key, const std::string& value) -> JSON::Value {
      self.params.set(key.c_str(), value.c_str());
      return JSON::makeNative(script_api::wrap(&self));
    };

    cls.addMethod("clearParameters") = [](CommandObject& self) -> JSON::Value {
      self.params.clear();
      return JSON::makeNative(script_api::wrap(&self));
    };

    // One method per registered command, named by the command id.
    for (auto cmd : *app::CommandsModule::instance()) {
      cls.addMethod(cmd->id()) = [cmd](CommandObject& self, JSON::Value& mapArg) -> JSON::Value {
        app::UIContext* ctx = app::UIContext::instance();
        if (!ctx)
          return JSON::Value{0.0};
        if (!cmd->isEnabled(ctx))
          return JSON::Value{0.0};
        if (mapArg.isObject()) {
          // Transient params: set, execute, then clear so they don't leak.
          for (auto& [k, v] : mapArg.object())
            self.params.set(k.c_str(), v.toString().c_str());
          ctx->executeCommand(cmd, self.params);
          self.params.clear();
        } else {
          ctx->executeCommand(cmd, self.params);
        }
        return JSON::Value{1.0};
      };
    }
  }

  std::string init(const std::string&, JSON::Value&) override {
    return "globalThis.command = new Command();";
  }
};

static di::provide<Extension, CommandExtension> commandExt{"command"};
