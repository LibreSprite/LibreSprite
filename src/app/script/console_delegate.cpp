// LibreSprite
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/console.h"
#include "script/engine_delegate.h"

class ConsoleEngineDelegate : public script::EngineDelegate {
public:
  void onConsolePrint(const char* text) override {
    m_console.printf("%s\n", text);
  }

private:
    app::Console m_console;
};

static script::EngineDelegate::Regular<ConsoleEngineDelegate> reg("gui");
