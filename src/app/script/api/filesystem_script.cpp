// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// Filesystem extension: the fs API on top of LibreSprite's TaskManager.
// Blocking I/O runs on a worker thread; the callback is delivered on the
// main thread, where the JS Promise is resolved. Relative paths resolve
// against the running script's directory; absolute paths pass through.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "delta/StringUtils.hpp"
#include "di.hpp"

#include "app/script/app_scripting.h"
#include "app/script/script_permissions.h"
#include "app/task_manager.h"
#include "base/fs.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(ANDROID)
extern std::string _AndroidDataDir;
extern void _AndroidShareFile(const std::string& path);
#endif

namespace {

  // Absolute paths pass through; relative paths resolve against the
  // running script's directory (CWD when no script is active).
  std::string resolvePath(const std::string& path) {
    std::filesystem::path p{path};
    if (p.is_absolute())
      return path;
    std::string base = std::filesystem::current_path().string();
    std::string script = app::AppScripting::getFileName();
    if (!script.empty()) {
      auto parent = std::filesystem::path(script).parent_path();
      if (!parent.empty())
        base = parent.string();
    }
    return (std::filesystem::path(base) / path).lexically_normal().string();
  }

  JSON::Value errorResult(const std::string& message) {
    JSON::Value rsp;
    rsp.object();
    rsp["error"] = message;
    return rsp;
  }

  // The active script lacks the required permission: the operation fails
  // with the standard error object (the JS promise rejects).
  JSON::Value deniedOp(const char* perm) {
    return errorResult(app::ScriptPermissions::denyMessage(perm));
  }

  // The file_time_type clock is implementation-defined; rebase onto
  // system_clock for a portable unix timestamp.
  double mtimeToUnixSeconds(const std::filesystem::file_time_type& mtime) {
    auto lastModified = std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            mtime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now()));
    return static_cast<double>(lastModified);
  }

  // The blocking operations below run on a TaskManager worker thread and
  // return a JSON object with an "error" key on failure.

  JSON::Value readFileOp(const std::string& path, bool binary, bool base64) {
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec) && !ec)
      return errorResult("Path is a directory: " + path);

    std::ifstream file(path, std::ios::binary);
    if (!file.good())
      return errorResult("Unable to open file for reading: " + path);

    std::string content{(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()};
    if (binary) {
      if (base64)
        return JSON::Value{su::btoa(content)};
      return JSON::Value{std::make_shared<std::vector<uint8_t>>(content.begin(), content.end())};
    }
    return JSON::Value{std::string{std::move(content)}};
  }

  JSON::Value writeFileOp(const std::string& path, const std::string& data) {
    std::error_code ec;
    auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
      std::filesystem::create_directories(parent, ec);
      if (ec)
        return errorResult("Failed to create parent directories for " + path + ": " + ec.message());
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.good())
      return errorResult("Unable to open file for writing: " + path);

    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file.good())
      return errorResult("Failed while writing file: " + path);

    JSON::Value rsp;
    rsp.object();
    rsp["bytesWritten"] = static_cast<double>(data.size());
    return rsp;
  }

  JSON::Value statOp(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
      return errorResult("Path does not exist: " + path);

    JSON::Value rsp;
    auto& obj = rsp.object();
    obj["isFile"] = std::filesystem::is_regular_file(path, ec);
    obj["isDirectory"] = std::filesystem::is_directory(path, ec);
    if (ec)
      return errorResult("Failed to stat path: " + path);

    if (obj["isFile"].boolean())
      obj["size"] = static_cast<double>(std::filesystem::file_size(path, ec));
    else
      obj["size"] = 0.0;
    if (ec)
      return errorResult("Failed to get file size: " + path);

    auto mtime = std::filesystem::last_write_time(path, ec);
    if (ec)
      return errorResult("Failed to get modified time: " + path);
    obj["lastModified"] = mtimeToUnixSeconds(mtime);
    obj["path"] = path;
    return rsp;
  }

  JSON::Value listDirOp(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec) || ec)
      return errorResult(std::filesystem::exists(path, ec)
          ? "Path is not a directory: " + path
          : "Path does not exist: " + path);

    JSON::Value rsp;
    auto& obj = rsp.object();
    auto& arr = obj["entries"].array();

    auto begin = std::filesystem::directory_iterator(path, ec);
    auto end = std::filesystem::directory_iterator{};
    if (ec)
      return errorResult("Failed to iterate directory " + path + ": " + ec.message());

    for (auto it = begin; it != end; ++it) {
      if (ec)
        return errorResult("Failed to iterate directory " + path + ": " + ec.message());
      const auto& entry = *it;
      arr.push_back({});
      auto& out = arr.back().object();
      out["name"] = entry.path().filename().string();
      out["isDirectory"] = entry.is_directory(ec);
      out["isFile"] = entry.is_regular_file(ec);
      if (ec)
        return errorResult("Failed to read entry type in " + path + ": " + ec.message());
      if (out["isFile"].boolean())
        out["size"] = static_cast<double>(entry.file_size(ec));
      else
        out["size"] = 0.0;
      if (ec)
        return errorResult("Failed to read file size in " + path + ": " + ec.message());
      out["lastModified"] = mtimeToUnixSeconds(entry.last_write_time(ec));
      if (ec)
        return errorResult("Failed to read modified time in " + path + ": " + ec.message());
    }
    return rsp;
  }

  JSON::Value copyFileOp(const std::string& srcPath, const std::string& dstPath, bool append) {
    std::ifstream src(srcPath, std::ios::binary);
    if (!src.good())
      return errorResult("Unable to open source for reading: " + srcPath);

    std::error_code ec;
    auto parent = std::filesystem::path(dstPath).parent_path();
    if (!parent.empty()) {
      std::filesystem::create_directories(parent, ec);
      if (ec)
        return errorResult("Failed to create destination directories for " + dstPath + ": " + ec.message());
    }

    std::ofstream dst(dstPath, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    if (!dst.good())
      return errorResult("Unable to open destination for writing: " + dstPath);

    std::vector<char> buffer(64 * 1024);
    std::size_t bytesCopied = 0;
    std::streamsize count;
    do {
      src.read(buffer.data(), buffer.size());
      count = src.gcount();
      if (count > 0 && !dst.write(buffer.data(), count))
        return errorResult("Failed while copying to " + dstPath);
      bytesCopied += static_cast<std::size_t>(count);
    } while (count == static_cast<std::streamsize>(buffer.size()));

    JSON::Value rsp;
    rsp.object();
    rsp["bytesCopied"] = static_cast<double>(bytesCopied);
    rsp["source"] = srcPath;
    rsp["destination"] = dstPath;
    rsp["append"] = append;
    return rsp;
  }

  JSON::Value deleteFileOp(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
      return errorResult("Path does not exist: " + path);

    if (std::filesystem::remove(path, ec)) {
      JSON::Value rsp;
      rsp.object();
      rsp["path"] = path;
      rsp["deleted"] = true;
      return rsp;
    }
    return errorResult(ec
        ? "Failed to delete " + path + ": " + ec.message()
        : "Failed to delete " + path);
  }

} // namespace

class FileSystemExtension : public Extension {
public:
  FileSystemExtension() {
    // __fsReadFile(path, options, callback) -> undefined.
    // Async: resolves with a string (text), a Uint8Array (binary) or a
    // base64 string (base64); rejects with {error}.
    addFunction("__fsReadFile") = [this](const std::string& rawPath, JSON::Value& options, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsRead)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsRead); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string path = resolvePath(rawPath);
      bool binary = false, base64 = false;
      if (options.isObject()) {
        auto& obj = options.object();
        auto encoding = obj["encoding"].string("text");
        binary = encoding == "binary" || encoding == "base64";
        base64 = encoding == "base64";
        if (obj["binary"].boolean())
          binary = true;
        if (obj["base64"].boolean())
          base64 = true;
      }
      runAsync(cb, [path, binary, base64]() {
        return readFileOp(path, binary, base64);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsWriteFile(path, content, options, callback) -> undefined.
    // Async: content is a string or a Uint8Array; a base64 option decodes
    // it first. Resolves with the bytes written; rejects with {error}.
    addFunction("__fsWriteFile") = [this](const std::string& rawPath, JSON::Value& content, JSON::Value& options, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsWrite)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsWrite); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string path = resolvePath(rawPath);
      std::string data;
      if (content.isString()) {
        data = content.string();
      } else if (content.isByteArray()) {
        auto& vec = content.byteArray();
        data.assign(reinterpret_cast<const char*>(vec.data()), vec.size());
      }
      if (options.isObject()) {
        auto& obj = options.object();
        if ((obj.count("encoding") && obj["encoding"].string() == "base64") ||
            (obj.count("base64") && obj["base64"].boolean()))
          data = su::atob(data);
      }
      runAsync(cb, [path, data = std::move(data)]() {
        return writeFileOp(path, data);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsStat(path, callback) -> undefined.
    // Async: resolves with {isFile, isDirectory, size, lastModified, path}.
    addFunction("__fsStat") = [this](const std::string& rawPath, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsRead)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsRead); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string path = resolvePath(rawPath);
      runAsync(cb, [path]() {
        return statOp(path);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsListDir(path, callback) -> undefined.
    // Async: resolves with {entries: [{name, isFile, isDirectory, size, lastModified}]}.
    addFunction("__fsListDir") = [this](const std::string& rawPath, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsRead)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsRead); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string path = resolvePath(rawPath);
      runAsync(cb, [path]() {
        return listDirOp(path);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsCopyFile(src, dst, append, callback) -> undefined.
    // Async: resolves with {bytesCopied, source, destination, append}.
    addFunction("__fsCopyFile") = [this](const std::string& rawSrc, const std::string& rawDst, JSON::Value& appendV, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsWrite)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsWrite); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string src = resolvePath(rawSrc);
      std::string dst = resolvePath(rawDst);
      bool append = appendV.boolean();
      runAsync(cb, [src, dst, append]() {
        return copyFileOp(src, dst, append);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsDeleteFile(path, callback) -> undefined.
    // Async: resolves with {path, deleted: true}.
    addFunction("__fsDeleteFile") = [this](const std::string& rawPath, JSON::Value& callback) -> JSON::Value {
      if (!callback.isFunction())
        return JSON::Value{JSON::Special::Undefined};
      auto cb = std::get_if<JSON::FunctionRef>(&callback.value)->func;
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsWrite)) {
        runAsync(cb, []() { return deniedOp(app::ScriptPermissions::kFsWrite); });
        return JSON::Value{JSON::Special::Undefined};
      }
      std::string path = resolvePath(rawPath);
      runAsync(cb, [path]() {
        return deleteFileOp(path);
      });
      return JSON::Value{JSON::Special::Undefined};
    };

    // __fsShareFile(path) -> true. Synchronous: hands the file to the
    // system share sheet. Throws on platforms without one.
    addFunction("__fsShareFile") = [](const std::string& rawPath) -> JSON::Value {
      if (!app::ScriptPermissions::check(app::ScriptPermissions::kFsRead))
        throw std::runtime_error(app::ScriptPermissions::denyMessage(app::ScriptPermissions::kFsRead));
#if defined(ANDROID)
      std::string path = resolvePath(rawPath);
      std::error_code ec;
      if (!std::filesystem::is_regular_file(path, ec) || ec)
        throw std::runtime_error("shareFile: file not found: " + path);
      _AndroidShareFile(path);
      return JSON::Value{true};
#else
      throw std::runtime_error("shareFile is not supported on this platform");
#endif
    };

    addFunction("__fsGetCurrentPath") = []() -> JSON::Value {
      return JSON::Value{std::filesystem::current_path().string()};
    };

    addFunction("__fsGetExecutablePath") = []() -> JSON::Value {
#if defined(__EMSCRIPTEN__)
      // No executable on the web build.
      return JSON::Value{std::string{}};
#else
      // On Android this is the app data directory.
      return JSON::Value{base::get_app_path()};
#endif
    };

    addFunction("__fsGetCachePath") = []() -> JSON::Value {
#if defined(ANDROID)
      // The app cache directory (TMPDIR//tmp is not reliably writable).
      return JSON::Value{_AndroidDataDir};
#else
      return JSON::Value{base::get_temp_path()};
#endif
    };
  }

  void shutdown() override {
    // Release the JS callbacks of in-flight operations on the main thread,
    // before the engine is destroyed: the task objects that own them are
    // destroyed later (possibly on worker threads).
    std::lock_guard<std::mutex> guard(callbackMutex);
    pendingCallbacks.clear();
  }

  std::string init(const std::string& language, JSON::Value& settings) override {
    if (language != "js")
      return "";
    return R"(
globalThis.fs = {};

fs.readFile = function(path, options) {
return new Promise((ok, nok) => {
    const callback = (response) => {
        if (typeof response === 'string' || (response && response instanceof Uint8Array)) {
            ok(response);
        } else {
            nok(new Error((response && response.error) || "Failed to read file"));
        }
    };
    __fsReadFile(path, options || {}, callback);
});
};

fs.stat = function(path) {
return new Promise((ok, nok) => {
    __fsStat(path, (response) => {
        if (response && !response.error) {
            ok(response);
        } else {
            nok(new Error((response && response.error) || "Failed to stat path"));
        }
    });
});
};

fs.writeFile = function(path, content, options) {
return new Promise((ok, nok) => {
    const callback = (response) => {
        if (response && !response.error) {
            ok(response.bytesWritten || 0);
        } else {
            nok(new Error((response && response.error) || "Failed to write file"));
        }
    };
    __fsWriteFile(path, content || "", options || {}, callback);
});
};

fs.copyFile = function(srcPath, destPath, append) {
return new Promise((ok, nok) => {
    __fsCopyFile(srcPath, destPath, !!append, (response) => {
        if (response && !response.error) {
            ok(response.bytesCopied || 0);
        } else {
            nok(new Error((response && response.error) || "Failed to copy file"));
        }
    });
});
};

fs.listDir = function(path) {
return new Promise((ok, nok) => {
    __fsListDir(path, (response) => {
        if (response && !response.error) {
            ok(response.entries || []);
        } else {
            nok(new Error((response && response.error) || "Failed to list directory"));
        }
    });
});
};

fs.deleteFile = function(path) {
return new Promise((ok, nok) => {
    __fsDeleteFile(path, (response) => {
        if (response && !response.error) {
            ok(response);
        } else {
            nok(new Error((response && response.error) || "Failed to delete path"));
        }
    });
});
};

// Synchronous: sends the file to the system share sheet (Android only).
// Throws on other platforms.
fs.shareFile = function(path) {
    return __fsShareFile(path);
};

fs.getCurrentPath = function() {
    return __fsGetCurrentPath();
};

fs.getExecutablePath = function() {
    return __fsGetExecutablePath();
};

fs.getCachePath = function() {
    return __fsGetCachePath();
};
)";
  }

private:
  // JS callbacks of in-flight operations, released on the main thread (see
  // shutdown() and runAsync): the task object owning a lambda is destroyed
  // on a worker thread, and the QuickJS value must not be freed there.
  std::mutex callbackMutex;
  std::vector<std::shared_ptr<JSON::Function>> pendingCallbacks;

  void trackCallback(const std::shared_ptr<JSON::Function>& cb) {
    std::lock_guard<std::mutex> guard(callbackMutex);
    pendingCallbacks.push_back(cb);
  }

  void untrackCallback(const std::shared_ptr<JSON::Function>& cb) {
    std::lock_guard<std::mutex> guard(callbackMutex);
    pendingCallbacks.erase(std::remove_if(pendingCallbacks.begin(), pendingCallbacks.end(),
      [&cb](const auto& p) { return p == cb; }),
      pendingCallbacks.end());
  }

  // Runs op on a TaskManager worker thread and invokes the JS callback on
  // the main thread with the resulting object.
  template<typename Op>
  void runAsync(std::shared_ptr<JSON::Function> cb, Op op) {
    trackCallback(cb);
    std::weak_ptr<JSON::Function> cbWeak{cb};
    app::TaskManager::instance().addTask<JSON::Value>(
      [op = std::move(op)]() mutable -> JSON::Value {
        return op();
      },
      [this, cbWeak](JSON::Value&& rsp) {
        if (auto fn = cbWeak.lock()) {
          JSON::Array args{std::move(rsp)};
          (*fn)(args);
          untrackCallback(fn);
        }
      },
      []{ /* nothing to abort */ });
  }
};

static di::provide<Extension, FileSystemExtension> fsExt{"filesystem"};
