// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/file/file_format.h"
#include "delta/JSON.hpp"

#include <string>

namespace app {

  // A file format registered at runtime by a script via
  // file.registerFormat(). The codec runs in the script: load/save raise
  // a format_load/format_save event (with a request id) to the script,
  // which replies through the "file" API (file.onLoad/...); the request
  // tracking and the watchdog live in script_codec_requests.h.
  class ScriptFileFormat : public FileFormat {
  public:
    ScriptFileFormat(const std::string& scriptPath,
                     const std::string& name,
                     const std::string& extensions,
                     bool load, bool save);

    // The canonical path of the script that registered this format
    // (dispatch routes load/save events back to it).
    const std::string& scriptPath() const { return m_scriptPath; }

  protected:
    const char* onGetName() const override { return m_name.c_str(); }
    const char* onGetExtensions() const override { return m_extensions.c_str(); }
    int onGetFlags() const override { return m_flags; }

    bool onLoad(FileOp* fop) override;
    bool onSave(FileOp* fop) override;

  private:
    // Raises the codec event and waits for the reply (the "file" API or
    // the watchdog settles it on the main thread).
    JSON::Value dispatchCodec(FileOp* fop, const char* eventName, JSON::Value payload);

    std::string m_scriptPath;
    std::string m_name;
    std::string m_extensions;
    int m_flags;
  };

} // namespace app
