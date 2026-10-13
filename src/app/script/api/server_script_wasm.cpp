// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// Web build stub for the HTTPServer extension. The browser has no sockets,
// so the class exists for API compatibility but does nothing: the
// constructor records an error, start() returns 0, and the other members
// report that state. The native build uses server_script.cpp instead.

#if defined(__EMSCRIPTEN__)

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"

#include <memory>
#include <string>

class Server {
public:
  explicit Server(JSON::Value&) {
    error_ = "HTTPServer is not supported in the web build";
  }

  int start() { return 0; }
  void stop() {}
  bool addVirtualFile(const std::string&, const std::string&, const std::string&) { return false; }
  int port() const { return 0; }
  std::string error() const { return error_; }

private:
  std::string error_;
};

class ServerExtension : public Extension {
public:
  ServerExtension() {
    auto& cls = addClass<void, Server>("HTTPServer");
    cls.setConstructor() = [](JSON::Value& opts) -> std::shared_ptr<Server> {
      return std::make_shared<Server>(opts);
    };
    cls.addMethod("start") = [](Server& self) -> JSON::Value {
      return JSON::Value{(double)self.start()};
    };
    cls.addMethod("stop") = [](Server& self) -> JSON::Value {
      self.stop();
      return JSON::Value{JSON::Special::Undefined};
    };
    cls.addMethod("addVirtualFile") = [](Server& self, const std::string& path,
      const std::string& content, const std::string& contentType) -> JSON::Value {
      return JSON::Value{self.addVirtualFile(path, content, contentType)};
    };
    cls.addGetter("port") = [](Server& self) -> JSON::Value {
      return JSON::Value{(double)self.port()};
    };
    cls.addGetter("error") = [](Server& self) -> JSON::Value {
      return self.error();
    };
  }
};

static di::provide<Extension, ServerExtension> serverExt{"server"};

#endif // __EMSCRIPTEN__
