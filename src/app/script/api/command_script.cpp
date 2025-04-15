// LibreSprite
// Copyright (C) 2024  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ui_context.h"
#include "script/engine.h"

class CommandScriptObject : public script::ScriptObject {
public:
  app::Params params;
  CommandScriptObject() {
    addFunction("setParameter", [this](const std::string& key, const std::string& value) {
      params.set(key.c_str(), value.c_str());
      return this;
    });

    addFunction("clearParameters", [this]() {params.clear(); return this;});

    for (auto cmd : *app::CommandsModule::instance()) {
        addFunction(cmd->id(), [this, cmd](script::Value::Map::data_t* map = nullptr){
            app::UIContext* ctx = app::UIContext::instance();
            if (!ctx)
                return 0;
            if (!cmd->isEnabled(ctx))
                return 0;
	    if (map) {
		for (auto& entry : *map) {
		    params.set(entry.first.c_str(), entry.second.str().c_str());
		}
	    }
            ctx->executeCommand(cmd, params);
	    if (map) {
		params.clear();
	    }
            return 1;
        }).doc(cmd->friendlyName());
    }
  }
};

static script::ScriptObject::Regular<CommandScriptObject> reg("command");
