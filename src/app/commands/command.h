// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/commands/command_factory.h"
#include <string>

namespace app {
  class Context;
  class Params;

  enum CommandFlags {
    CmdUIOnlyFlag     = 0x00000001,
    CmdRecordableFlag = 0x00000002,
  };

  class Command {
  public:
    Command(const char* id, const char* friendlyName, CommandFlags flags);
    virtual ~Command();

    virtual Command* clone() const { return new Command(*this); }

    const std::string& id() const { return m_id; }
    std::string friendlyName() const;

    void loadParams(const Params& params);
    bool isEnabled(Context* context);
    bool isChecked(Context* context);
    void execute(Context* context);

  protected:
    virtual void onLoadParams(const Params& params);
    virtual bool onEnabled(Context* context);
    virtual bool onChecked(Context* context);
    virtual void onExecute(Context* context);
    virtual std::string onGetFriendlyName() const;

  private:
    std::string m_id;
    std::string m_friendlyName;
    CommandFlags m_flags;
  };

} // namespace app
