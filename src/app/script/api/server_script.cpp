// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// HTTPServer extension: an HTTP server (cpp-httplib based) on top of
// LibreSprite's TaskManager. The server listens on a worker
// thread; each request is dispatched to the main thread (via
// TaskManager::delayed) where the JS handler runs, and the response is
// sent from the worker thread once the handler settles.
//
// Known limitations:
//  - A handler that never settles blocks an httplib worker thread until
//    the server is stopped.
//  - Handlers run on the main thread, so a long synchronous handler
//    freezes the UI (true of the whole script API).
//  - There is no per-request timeout.

#ifndef __EMSCRIPTEN__

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "delta/StringUtils.hpp"
#include "di.hpp"
#include "app/task_manager.h"
#include "base/fs.h"
#include "httplib/httplib.h"

#include <any>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

  // MIME types for static file serving.
  const std::unordered_map<std::string, std::string>& mimeTypes() {
    static const std::unordered_map<std::string, std::string> m = {
      {".html", "text/html"}, {".css", "text/css"},
      {".js", "application/javascript"}, {".png", "image/png"},
      {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
      {".gif", "image/gif"}, {".svg", "image/svg+xml"},
      {".json", "application/json"}, {".txt", "text/plain"},
      {".pdf", "application/pdf"}, {".zip", "application/zip"},
      {".mp3", "audio/mpeg"}, {".mp4", "video/mp4"},
    };
    return m;
  }

  // Removes JS references (functions, native objects, and the JS value memo
  // of objects) so the value is plain data: it is stored and destroyed on a
  // worker thread, and QuickJS values must only be freed on the main thread.
  // Nested references are dropped, matching toJSON's behavior for functions.
  void stripJsRefs(JSON::Value& v) {
    if (v.isFunction() || v.isNative()) {
      v = JSON::Value{JSON::Special::Undefined};
      return;
    }
    if (v.isObject()) {
      auto& obj = v.object();
      obj.memo = std::any{};
      auto& map = obj.map;
      for (auto it = map.begin(); it != map.end();) {
        if (it->second.isFunction() || it->second.isNative())
          it = map.erase(it);
        else {
          stripJsRefs(it->second);
          ++it;
        }
      }
      return;
    }
    if (v.isArray()) {
      auto& arr = v.array();
      for (auto it = arr.begin(); it != arr.end();) {
        if (it->isFunction() || it->isNative())
          it = arr.erase(it);
        else {
          stripJsRefs(*it);
          ++it;
        }
      }
    }
  }

} // namespace

class Server;
class ServerExtension;

// Maps a bound port to a running Server so __setServerResponse (which only
// knows the port) can find it. Holds weak_ptrs: an expired entry means the
// server was destroyed, so a stale entry is harmless.
struct ServerRegistry {
  std::mutex mutex;
  std::map<int, std::weak_ptr<Server>> servers;

  void registerServer(int port, const std::weak_ptr<Server>& server);
  void unregisterServer(int port);
  std::shared_ptr<Server> find(int port);
};

class Server : public std::enable_shared_from_this<Server> {
public:
  Server(JSON::Value& opts, ServerRegistry* registry, ServerExtension* ext);
  ~Server();

  // Binds and starts the server. Returns the bound port, or 0 on failure
  // (the reason is in error()). Idempotent while running.
  int start();
  // Stops the server. Idempotent; start() may be called again afterwards.
  void stop();
  // Adds a virtual file served for exact path matches (GET/HEAD).
  bool addVirtualFile(const std::string& path, const std::string& content,
                      const std::string& contentType);
  int port() const { return boundPort; }
  std::string error() const { return error_; }

  // Delivers a handler response on the main thread (from __setServerResponse).
  void setServerResponse(std::uint64_t reqId, JSON::Value rsp, JSON::Value error);

private:
  // The server thread: runs the blocking accept/serve loop.
  void serve();
  // Registers the httplib routes.
  void registerRoutes();
  // Dispatches one request to a method (static files for GET, else the
  // handler, else 404). Runs on an httplib worker thread.
  void route(const std::string& method, const httplib::Request& req, httplib::Response& res);
  // Invokes the JS handler for a method and waits for its response.
  void handleRequest(const std::string& method, const httplib::Request& req, httplib::Response& res);
  // Serves a virtual or disk file. Returns true if a response was sent.
  bool tryServeStatic(const httplib::Request& req, httplib::Response& res);
  void applyDefaultHeaders(httplib::Response& res) const;
  void applyResponse(httplib::Response& res, JSON::Value response, const std::string& scriptError);
  JSON::Value* getHandlerPtr(const std::string& method);
  JSON::Value getHandler(const std::string& method);

  // Options (set at construction, main thread).
  int requestedPort = 0;
  std::string host = "localhost";
  std::string staticFilePath;
  std::map<std::string, std::string> defaultHeaders;
  JSON::Value onGet, onPost, onPut, onDelete, onPatch, onOptions;

  // Lifecycle (touched on the main thread only).
  int boundPort = 0;
  std::string error_;
  bool started = false;
  std::atomic_bool running{false};
  std::thread serverThread;

  // Virtual files (read on worker threads, written on the main thread).
  struct VirtualFile { std::string content; std::string contentType; };
  std::map<std::string, VirtualFile> virtualFiles;
  std::shared_mutex virtualFilesMutex;

  // In-flight requests (inserted/erased on worker threads, resolved on the
  // main thread; the worker blocks on cv until done or the server stops).
  struct PendingRequest {
    std::mutex mtx;
    std::condition_variable cv;
    JSON::Value response;
    std::string scriptError;
    bool done = false;
  };
  std::map<std::uint64_t, std::unique_ptr<PendingRequest>> pendingRequests;
  std::mutex pendingMutex;
  std::atomic<std::uint64_t> requestCounter{0};

  httplib::Server server;
  ServerRegistry* registry = nullptr;
  ServerExtension* ext = nullptr;
};

class ServerExtension : public Extension {
public:
  ServerExtension() {
    auto& cls = addClass<void, Server>("HTTPServer");
    cls.setConstructor() = [this](JSON::Value& opts) -> std::shared_ptr<Server> {
      return std::make_shared<Server>(opts, &registry, this);
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

    // __setServerWrapper(fn): stores the boot-script wrapper that invokes a
    // user handler and reports the result via __setServerResponse.
    addFunction("__setServerWrapper") = [this](JSON::Value& fn) -> JSON::Value {
      if (fn.isFunction())
        wrapper = std::make_shared<JSON::Value>(fn);
      return JSON::Value{JSON::Special::Undefined};
    };

    // __setServerResponse(port, reqId, rsp, error): delivers a handler result
    // (error is a string on failure, undefined on success).
    addFunction("__setServerResponse") = [this](double port, double reqId,
      JSON::Value& rsp, JSON::Value& error) -> JSON::Value {
      if (auto srv = registry.find((int)port))
        srv->setServerResponse((std::uint64_t)reqId, rsp, error);
      return JSON::Value{JSON::Special::Undefined};
    };
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return R"(
__setServerWrapper(
async (port, userCB, req) => {
  try {
    __setServerResponse(port, req.id, await userCB(req));
  } catch (ex) {
    __setServerResponse(port, req.id, null, ex + '\n' + (ex.stack || ""));
  }
});
)";
  }

  void shutdown() override {
    // Stop the running servers on the main thread before the engine (and its
    // JS-owned Server objects) is destroyed, so their threads are joined first.
    std::vector<std::shared_ptr<Server>> servers;
    {
      std::lock_guard lock(registry.mutex);
      for (auto& [_, weak] : registry.servers)
        if (auto srv = weak.lock())
          servers.push_back(srv);
      registry.servers.clear();
    }
    for (auto& srv : servers)
      srv->stop();
    // Release the stored wrapper (a JS reference) on the main thread.
    wrapper.reset();
  }

  // A listening server keeps the headless pump running.
  bool keepAlive() override {
    std::lock_guard lock(registry.mutex);
    for (auto& [_, weak] : registry.servers)
      if (weak.lock())
        return true;
    return false;
  }

  ServerRegistry registry;

  // A weak reference to the boot-script wrapper (set once via
  // __setServerWrapper, before any server is created).
  std::weak_ptr<JSON::Value> wrapperRef() {
    return wrapper;
  }

private:
  // The boot-script wrapper. A JS reference: released on the main thread.
  std::shared_ptr<JSON::Value> wrapper;
};

// --- ServerRegistry (Server must be complete for weak_ptr::lock) ---

void ServerRegistry::registerServer(int port, const std::weak_ptr<Server>& server) {
  std::lock_guard lock(mutex);
  servers[port] = server;
}

void ServerRegistry::unregisterServer(int port) {
  std::lock_guard lock(mutex);
  servers.erase(port);
}

std::shared_ptr<Server> ServerRegistry::find(int port) {
  std::lock_guard lock(mutex);
  auto it = servers.find(port);
  if (it == servers.end())
    return nullptr;
  return it->second.lock();
}

// --- Server ---

Server::Server(JSON::Value& opts, ServerRegistry* reg, ServerExtension* e) : registry{reg}, ext{e} {
  // httplib's default socket options set SO_REUSEPORT, which would let a
  // second server silently share an already-bound port instead of failing.
  // Set only SO_REUSEADDR: a busy port still fails to bind, but a port in
  // TIME_WAIT can be re-bound after stop() (stop-then-start).
  server.set_socket_options([](::socket_t sock) {
    // Reuse httplib's wrapper: on Windows setsockopt() takes a const char*.
    httplib::detail::set_socket_opt(sock, SOL_SOCKET, SO_REUSEADDR, 1);
  });
  if (opts.isObject()) {
    auto& o = opts.object();
    if (auto it = o.find("port"); it != o.end() && it->second.isNumber())
      requestedPort = (int)it->second.number();
    if (auto it = o.find("host"); it != o.end() && it->second.isString())
      host = it->second.string();
    if (auto it = o.find("staticFilePath"); it != o.end() && it->second.isString())
      staticFilePath = it->second.string();
    if (auto it = o.find("defaultHeaders"); it != o.end() && it->second.isObject()) {
      for (auto& [k, v] : it->second.object())
        defaultHeaders[k] = v.string();
    }
    if (auto it = o.find("onGet"); it != o.end() && it->second.isFunction())
      onGet = it->second;
    if (auto it = o.find("onPost"); it != o.end() && it->second.isFunction())
      onPost = it->second;
    if (auto it = o.find("onPut"); it != o.end() && it->second.isFunction())
      onPut = it->second;
    if (auto it = o.find("onDelete"); it != o.end() && it->second.isFunction())
      onDelete = it->second;
    if (auto it = o.find("onPatch"); it != o.end() && it->second.isFunction())
      onPatch = it->second;
    if (auto it = o.find("onOptions"); it != o.end() && it->second.isFunction())
      onOptions = it->second;
  }
  registerRoutes();
}

Server::~Server() {
  stop();
}

int Server::start() {
  if (started)
    return boundPort;

  error_ = "";
  running = false;

  // Another of our servers may already hold this port. "localhost" resolves
  // to both ::1 and 127.0.0.1, so a plain re-bind would otherwise succeed on
  // the other family; check the registry so one port = one server.
  const bool portTaken =
    requestedPort > 0 && registry && registry->find(requestedPort);

  // bind_to_port returns immediately; run it here (main thread) so the
  // server thread only ever owns a bound socket (stop() then always closes
  // it cleanly). A failed bind sets httplib's is_decommissioned flag, so
  // reset it (via stop(), a no-op while not running) before each attempt.
  int bound = 0;
  bool ok = false;
  if (requestedPort == 0) {
    for (int p = 9000; p <= 9099 && !ok; ++p) {
      if (registry && registry->find(p))
        continue;  // one of our servers already uses this port
      server.stop();
      ok = server.bind_to_port(host, p);
      if (ok)
        bound = p;
    }
    if (!ok)
      error_ = "No free port found in range 9000-9099";
  } else if (portTaken) {
    error_ = "Port " + std::to_string(requestedPort) + " is already in use";
  } else {
    server.stop();
    ok = server.bind_to_port(host, requestedPort);
    if (ok)
      bound = requestedPort;
    else
      error_ = "Port " + std::to_string(requestedPort) + " is already in use";
  }

  if (!ok)
    return 0;

  running = true;
  serverThread = std::thread(&Server::serve, this);
  // Wait (briefly) for the serve thread to enter the listening loop. stop()
  // only closes the socket once httplib's is_running_ is set, so without this
  // a stop() right after start() would join a thread still about to listen.
  for (int i = 0; i < 1000 && !server.is_running(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  boundPort = bound;
  started = true;
  if (registry)
    registry->registerServer(boundPort, weak_from_this());
  return boundPort;
}

void Server::stop() {
  if (!started)
    return;
  started = false;
  running = false;

  // Wake handlers blocked waiting for a response so they can bail out.
  {
    std::lock_guard lock(pendingMutex);
    for (auto& [_, pending] : pendingRequests)
      pending->cv.notify_all();
  }

  // Close the socket; the serve thread's accept loop exits and it joins the
  // httplib worker pool (which is why the notify above must come first).
  server.stop();
  if (serverThread.joinable())
    serverThread.join();

  // The workers erased their own entries; drop anything left over.
  {
    std::lock_guard lock(pendingMutex);
    pendingRequests.clear();
  }

  if (registry)
    registry->unregisterServer(boundPort);
  boundPort = 0;
}

bool Server::addVirtualFile(const std::string& path, const std::string& content,
                            const std::string& contentType) {
  std::unique_lock lock(virtualFilesMutex);
  virtualFiles[path] = VirtualFile{content, contentType};
  return true;
}

void Server::serve() {
  server.listen_after_bind();
}

void Server::registerRoutes() {
  server.Get("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("GET", req, res);
  });
  server.Post("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("POST", req, res);
  });
  server.Put("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("PUT", req, res);
  });
  server.Delete("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("DELETE", req, res);
  });
  server.Patch("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("PATCH", req, res);
  });
  server.Options("(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
    route("OPTIONS", req, res);
  });
}

void Server::route(const std::string& method, const httplib::Request& req, httplib::Response& res) {
  // Static files (virtual, then disk) take priority for GET/HEAD; the onGet
  // handler is the fallback, matching the documented static-file behaviour.
  if (method == "GET" && tryServeStatic(req, res))
    return;
  if (JSON::Value* h = getHandlerPtr(method); h && h->isFunction()) {
    handleRequest(method, req, res);
    return;
  }
  applyDefaultHeaders(res);
  res.status = 404;
  res.set_content("Not Found", "text/plain");
}

void Server::handleRequest(const std::string& method, const httplib::Request& req, httplib::Response& res) {
  JSON::Value* handler = getHandlerPtr(method);
  if (!handler || !handler->isFunction())
    return;

  std::uint64_t reqId = requestCounter.fetch_add(1);
  auto pending = std::make_unique<PendingRequest>();
  PendingRequest* pendingPtr = pending.get();
  {
    std::lock_guard lock(pendingMutex);
    pendingRequests.emplace(reqId, std::move(pending));
  }

  // Build the request object. The body is a byte array (binary-safe); the
  // other fields are plain strings/numbers, so this is safe on a worker thread.
  JSON::Value reqObj;
  reqObj["path"] = req.path;
  reqObj["method"] = method;
  reqObj["body"].byteArray() = {req.body.begin(), req.body.end()};
  reqObj["id"] = (double)reqId;
  reqObj["headers"].object();
  for (auto& [k, v] : req.headers)
    reqObj["headers"][k] = v;
  reqObj["query"].object();
  for (auto& [k, v] : req.params)
    reqObj["query"][k] = v;

  // Run the JS handler on the main thread. The wrapper and handler hold JS
  // references, so they are captured as weak_ptrs here and copied only on the
  // main thread inside the task (a QuickJS value must not be copied off-thread).
  auto weakSelf = weak_from_this();
  auto wrapperWeak = ext->wrapperRef();
  const int port = boundPort;
  app::TaskManager::instance().delayed([weakSelf, wrapperWeak, port, reqId, method, reqObj]() {
    auto self = weakSelf.lock();
    auto wrapper = wrapperWeak.lock();
    if (!self || !wrapper)
      return;
    JSON::Value wrapperCopy = *wrapper;
    if (!wrapperCopy.isFunction())
      return;
    JSON::Value handlerCopy = self->getHandler(method);
    if (!handlerCopy.isFunction())
      return;
    JSON::Array args;
    args.emplace_back((double)port);
    args.emplace_back(std::move(handlerCopy));
    args.emplace_back(std::move(reqObj));
    wrapperCopy(args);
  });

  // Block until the handler reports a response or the server is stopped.
  {
    std::unique_lock lock(pendingPtr->mtx);
    pendingPtr->cv.wait(lock, [pendingPtr, this] {
      return pendingPtr->done || !running.load();
    });
  }

  if (!running.load()) {
    // Stopped while waiting: drop the request and send nothing.
    std::lock_guard lock(pendingMutex);
    pendingRequests.erase(reqId);
    return;
  }

  JSON::Value response;
  std::string scriptError;
  {
    std::lock_guard lock(pendingPtr->mtx);
    response = std::move(pendingPtr->response);
    scriptError = std::move(pendingPtr->scriptError);
  }
  {
    std::lock_guard lock(pendingMutex);
    pendingRequests.erase(reqId);
  }

  applyResponse(res, response, scriptError);
}

bool Server::tryServeStatic(const httplib::Request& req, httplib::Response& res) {
  std::string path = req.path;
  if (path == "/")
    path = "/index.html";

  // Virtual files (exact match).
  {
    std::shared_lock lock(virtualFilesMutex);
    auto it = virtualFiles.find(path);
    if (it != virtualFiles.end()) {
      applyDefaultHeaders(res);
      res.status = 200;
      res.set_content(it->second.content, it->second.contentType.c_str());
      return true;
    }
  }

  if (staticFilePath.empty())
    return false;

  // Reject paths that walk back out of staticFilePath.
  if (base::has_path_traversal(path)) {
    applyDefaultHeaders(res);
    res.status = 403;
    res.set_content("Forbidden", "text/plain");
    return true;
  }

  // Extension -> content type (unknown extension falls through to onGet).
  const std::size_t dot = path.find_last_of('.');
  const std::size_t slash = path.find_last_of('/');
  std::string fileExt = (dot != std::string::npos && (slash == std::string::npos || dot > slash))
    ? path.substr(dot) : "";
  auto mimeIt = mimeTypes().find(fileExt);
  if (mimeIt == mimeTypes().end())
    return false;

  std::ifstream file(staticFilePath + path, std::ios::binary);
  if (!file.good())
    return false;  // not found: fall through to onGet (SPA-style fallback)

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  applyDefaultHeaders(res);
  res.status = 200;
  res.set_content(content, mimeIt->second.c_str());
  return true;
}

void Server::applyDefaultHeaders(httplib::Response& res) const {
  for (auto& [k, v] : defaultHeaders)
    res.set_header(k, v);
}

void Server::applyResponse(httplib::Response& res, JSON::Value response, const std::string& scriptError) {
  applyDefaultHeaders(res);

  // A handler exception becomes a 500.
  if (!scriptError.empty()) {
    res.status = 500;
    res.set_content(scriptError, "text/plain");
    return;
  }

  // null -> 204 No Content.
  if (response.isNull()) {
    res.status = 204;
    return;
  }

  // number -> that status with an empty body.
  if (response.isNumber()) {
    res.status = (int)response.number();
    return;
  }

  // string -> 200 text/plain.
  if (response.isString()) {
    res.status = 200;
    res.set_content(response.string(), "text/plain");
    return;
  }

  // byte array -> 200 octet-stream.
  if (response.isByteArray()) {
    res.status = 200;
    auto& bytes = response.byteArray();
    res.set_content(std::string(bytes.begin(), bytes.end()), "application/octet-stream");
    return;
  }

  // object -> { status, body, headers } with full control; an object without
  // a "body" key is sent whole as the JSON body.
  if (response.isObject()) {
    auto& obj = response.object();
    int status = 200;
    std::string contentType;
    std::string body;

    if (auto it = obj.find("status"); it != obj.end() && it->second.isNumber())
      status = (int)it->second.number();

    if (auto it = obj.find("headers"); it != obj.end() && it->second.isObject()) {
      for (auto& [k, v] : it->second.object()) {
        if (su::toLower(k) == "content-type")
          contentType = v.string();
        else
          res.set_header(k, v.string());
      }
    }

    if (auto it = obj.find("body"); it != obj.end()) {
      if (it->second.isString()) {
        body = it->second.string();
        if (contentType.empty())
          contentType = "text/plain";
      } else if (it->second.isByteArray()) {
        auto& bytes = it->second.byteArray();
        body.assign(bytes.begin(), bytes.end());
        if (contentType.empty())
          contentType = "application/octet-stream";
      } else {
        body = it->second.toJSON();
        if (contentType.empty())
          contentType = "application/json";
      }
    } else {
      body = response.toJSON();
      if (contentType.empty())
        contentType = "application/json";
    }

    res.status = status;
    res.set_content(body, contentType.c_str());
    return;
  }

  // undefined / boolean: 200 with an empty body.
  res.status = 200;
}

JSON::Value* Server::getHandlerPtr(const std::string& method) {
  if (method == "GET") return &onGet;
  if (method == "POST") return &onPost;
  if (method == "PUT") return &onPut;
  if (method == "DELETE") return &onDelete;
  if (method == "PATCH") return &onPatch;
  if (method == "OPTIONS") return &onOptions;
  return nullptr;
}

JSON::Value Server::getHandler(const std::string& method) {
  auto* h = getHandlerPtr(method);
  return h ? *h : JSON::Value{JSON::Special::Undefined};
}

void Server::setServerResponse(std::uint64_t reqId, JSON::Value rsp, JSON::Value error) {
  // Runs on the main thread: drop any JS references the handler returned so
  // the stored value is plain data, safe to destroy on a worker thread.
  stripJsRefs(rsp);
  const std::string scriptError = error.isString() ? error.string() : "";
  std::lock_guard lock(pendingMutex);
  auto it = pendingRequests.find(reqId);
  if (it == pendingRequests.end())
    return;  // already handled (e.g. the server stopped)
  auto& pending = it->second;
  {
    std::lock_guard plock(pending->mtx);
    pending->response = std::move(rsp);
    pending->scriptError = std::move(scriptError);
    pending->done = true;
  }
  pending->cv.notify_one();
}

static di::provide<Extension, ServerExtension> serverExt{"server"};

#endif // __EMSCRIPTEN__
