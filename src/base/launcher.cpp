// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2018-2026  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/exception.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "base/path.h"
#include "base/string.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(ANDROID)
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace
{

// Runs argv[0] with the given arguments directly (no shell), so a file/URL
// containing shell metacharacters ($(...), backticks, quotes, backslashes)
// cannot be interpreted as a command. Returns 0 on success, like
// std::system did for the callers below.
int spawn_and_wait(const std::vector<std::string>& args)
{
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args)
    argv.push_back(const_cast<char*>(arg.c_str()));
  argv.push_back(nullptr);

  pid_t pid;
  if (posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ) != 0)
    return -1;

  int status = 0;
  if (waitpid(pid, &status, 0) < 0 || !WIFEXITED(status))
    return -1;
  return WEXITSTATUS(status);
}

// The fork+exec path cannot report an exec failure to the parent (the child
// just exits with 127), so check the executable up front: it must exist and
// be executable. Bare names are searched in PATH, like execvp. Returns an
// error message, or "" if the executable is fine.
std::string checkExecutable(const std::string& path)
{
  auto check = [](const std::filesystem::path& p) -> std::string {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec) || ec)
      return "No such file or directory";
    if (!std::filesystem::is_regular_file(p, ec) || ec)
      return "Permission denied";
    if (access(p.c_str(), X_OK) != 0)
      return strerror(errno);
    return "";
  };
  if (path.find('/') != std::string::npos)
    return check(path);

  std::string pathEnv;
  if (const char* env = getenv("PATH"))
    pathEnv = env;
  for (auto& dir : base::split(pathEnv, ':')) {
    if (dir.empty())
      continue;
    if (check(std::filesystem::path(dir) / path).empty())
      return "";
  }
  return "No such file or directory";
}

// Builds a null-terminated argv from the program path and its arguments.
std::vector<char*> toArgv(const std::string& path, const std::vector<std::string>& args)
{
  std::vector<char*> argv;
  argv.reserve(args.size() + 2);
  argv.push_back(const_cast<char*>(path.c_str()));
  for (auto& arg : args)
    argv.push_back(const_cast<char*>(arg.c_str()));
  argv.push_back(nullptr);
  return argv;
}

// Detached spawn (no waitpid), see spawn_process. If captureOutput, the
// child's stdout+stderr are redirected to a pipe (the parent keeps the read
// end); the pipe ends are closed in the child so EOF happens on its exit.
base::launcher::ProcessInfo spawnPosix(const std::string& path,
                                       const std::vector<std::string>& args,
                                       const std::string& cwd, bool captureOutput)
{
  base::launcher::ProcessInfo info;
  int pipefd[2] = {-1, -1};
  if (captureOutput && pipe(pipefd) != 0) {
    info.error = strerror(errno);
    return info;
  }

  auto argv = toArgv(path, args);
  pid_t pid = -1;
  bool spawned = false;

  if (!cwd.empty()) {
    auto err = checkExecutable(path);
    if (!err.empty()) {
      info.error = err;
      return info;
    }
    // No portable way to set the child's working directory with
    // posix_spawn: fork+chdir+exec (async-signal-safe calls only).
    pid = fork();
    if (pid == 0) {
      if (chdir(cwd.c_str()) == 0) {
        if (captureOutput) {
          dup2(pipefd[1], STDOUT_FILENO);
          dup2(pipefd[1], STDERR_FILENO);
          close(pipefd[0]);
          close(pipefd[1]);
        }
        execvp(argv[0], argv.data());
      }
      _exit(127);
    }
    spawned = (pid > 0);
  }

  if (!spawned) {
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (captureOutput) {
      posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
      posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
      posix_spawn_file_actions_addclose(&actions, pipefd[0]);
      posix_spawn_file_actions_addclose(&actions, pipefd[1]);
    }
    int ret = posix_spawn(&pid, path.c_str(), &actions, &attr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);
    spawned = (ret == 0 && pid > 0);
  }

  if (!spawned) {
    if (captureOutput) {
      close(pipefd[0]);
      close(pipefd[1]);
    }
    info.error = strerror(errno);
    return info;
  }

  info.pid = (int)pid;
  if (captureOutput) {
    close(pipefd[1]); // parent's copy of the write end
    info.outputPipeFd = pipefd[0];
  }
  return info;
}

bool terminatePosix(int pid)
{
  return kill(pid, SIGTERM) == 0;
}

std::optional<int> tryReapPosix(int pid, void*)
{
  int status = 0;
  if (waitpid(pid, &status, WNOHANG) != pid)
    return std::nullopt;
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return std::nullopt;
}

} // namespace
#endif

#if defined(ANDROID) || defined(__EMSCRIPTEN__)

namespace
{

base::launcher::ProcessInfo spawnStub(const std::string&, const std::vector<std::string>&,
                                      const std::string&, bool)
{
  base::launcher::ProcessInfo info;
  info.error = "not supported on this platform";
  return info;
}

bool terminateStub(int)
{
  return false;
}

std::optional<int> tryReapStub(int, void*)
{
  return std::nullopt;
}

} // namespace

#endif

#ifdef _WIN32
#include <windows.h>
#ifndef SEE_MASK_DEFAULT
#define SEE_MASK_DEFAULT 0x00000000
#endif

static int win32_shell_execute(const wchar_t* verb, const wchar_t* file, const wchar_t* params)
{
  SHELLEXECUTEINFOW sh;
  ZeroMemory((LPVOID)&sh, sizeof(sh));
  sh.cbSize = sizeof(sh);
  sh.fMask = SEE_MASK_DEFAULT;
  sh.lpVerb = verb;
  sh.lpFile = file;
  sh.lpParameters = params;
  sh.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteExW(&sh)) {
    int ret = GetLastError();
#if 0
    if (ret != 0) {
      DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;
      LPSTR msgbuf;

      if (FormatMessageA(flags, NULL, ret,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         reinterpret_cast<LPSTR>(&msgbuf),
                         0, NULL)) {
        ui::Alert::show("Problem<<Cannot open:<<%s<<%s||&Close", file, msgbuf);
        LocalFree(msgbuf);

        ret = 0;
      }
    }
#endif
    return ret;
  }
  else
    return 0;
}

namespace
{

// Quotes an argument for a CreateProcessW command line (MSVC quoting rules:
// backslashes before a quote are doubled, the quote is escaped).
std::wstring quoteArg(const std::wstring& arg)
{
  if (!arg.empty() && arg.find_first_of(L" \t\n\"") == std::wstring::npos)
    return arg;
  std::wstring out = L"\"";
  for (size_t i = 0; i < arg.size(); ) {
    size_t backslashes = 0;
    while (i < arg.size() && arg[i] == L'\\') {
      ++i;
      ++backslashes;
    }
    if (i < arg.size() && arg[i] == L'"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'"');
      ++i;
    }
    else {
      out.append(backslashes, L'\\');
      if (i < arg.size())
        out.push_back(arg[i++]);
    }
  }
  out.push_back(L'"');
  return out;
}

// Detached spawn (no wait), see spawn_process. If captureOutput, the
// child's stdout+stderr are redirected to a pipe (the parent keeps the read
// end); the write end is closed here so EOF happens when the child exits.
base::launcher::ProcessInfo spawnWin(const std::string& path,
                                     const std::vector<std::string>& args,
                                     const std::string& cwd, bool captureOutput)
{
  base::launcher::ProcessInfo info;
  HANDLE hRead = NULL, hWrite = NULL;
  if (captureOutput) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
      info.error = "CreatePipe failed (error " + std::to_string(GetLastError()) + ")";
      return info;
    }
  }

  std::wstring wpath = base::from_utf8(path);
  std::wstring cmdLine = quoteArg(wpath);
  for (auto& arg : args)
    cmdLine += L" " + quoteArg(base::from_utf8(arg));

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  if (captureOutput) {
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
  }

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  std::wstring wcwd = base::from_utf8(cwd);
  if (!CreateProcessW(wpath.c_str(), cmdLine.data(), NULL, NULL, captureOutput,
                      CREATE_NO_WINDOW, NULL, wcwd.empty() ? NULL : wcwd.c_str(),
                      &si, &pi)) {
    if (captureOutput) {
      CloseHandle(hRead);
      CloseHandle(hWrite);
    }
    info.error = "CreateProcess failed (error " + std::to_string(GetLastError()) + ")";
    return info;
  }

  CloseHandle(pi.hThread);
  if (captureOutput)
    CloseHandle(hWrite);

  info.pid = (int)pi.dwProcessId;
  info.processHandle = (void*)pi.hProcess;
  if (captureOutput)
    info.outputPipe = (void*)hRead;
  return info;
}

bool terminateWin(int pid)
{
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
  if (!hProcess)
    return false;
  bool ok = (TerminateProcess(hProcess, 1) != 0);
  CloseHandle(hProcess);
  return ok;
}

std::optional<int> tryReapWin(int, void* processHandle)
{
  HANDLE hProcess = (HANDLE)processHandle;
  if (WaitForSingleObject(hProcess, 0) != WAIT_OBJECT_0)
    return std::nullopt;
  DWORD exitCode = 0;
  GetExitCodeProcess(hProcess, &exitCode);
  CloseHandle(hProcess);
  return (int)exitCode;
}

} // namespace
#endif  // _WIN32

namespace base {
namespace launcher {

bool open_url(const std::string& url)
{
  return open_file(url);
}

bool open_file(const std::string& file)
{
  int ret = -1;

#ifdef __EMSCRIPTEN__

  auto protocol = base::string_to_lower(base::split(file, ':')[0]);
  if (protocol == "https" || protocol == "http") {
      MAIN_THREAD_EM_ASM({
	      open(UTF8ToString($0));
	  }, file.c_str());
      ret = 0;
  }

#elif _WIN32

  ret = win32_shell_execute(L"open",
                            base::from_utf8(file).c_str(), NULL);

#elif __APPLE__

  ret = spawn_and_wait({"open", file});

#elif ANDROID

  // No-op

#else

  ret = spawn_and_wait({"xdg-open", file});

#endif

  return (ret == 0);
}

bool open_folder(const std::string& _file)
{
  std::string file = base::fix_path_separators(_file);

#ifdef _WIN32

  int ret;
  if (base::is_directory(file)) {
    ret = win32_shell_execute(NULL, L"explorer",
      (L"/n,/e,\"" + base::from_utf8(file) + L"\"").c_str());
  }
  else {
    ret = win32_shell_execute(NULL, L"explorer",
      (L"/e,/select,\"" + base::from_utf8(file) + L"\"").c_str());
  }
  return (ret == 0);

#elif __APPLE__

  int ret;
  if (base::is_directory(file)) {
    ret = spawn_and_wait({"open", file});
  }
  else {
    ret = spawn_and_wait({"open", "--reveal", file});
  }
  return (ret == 0);

#elif defined(ANDROID) || defined(__EMSCRIPTEN__)

  // No-op
  return false;

#else

  if (!base::is_directory(file))
    file = base::get_file_path(file);

  int ret = spawn_and_wait({"xdg-open", file});
  return (ret == 0);

#endif
}

ProcessInfo spawn_process(const std::string& path, const std::vector<std::string>& args,
                          const std::string& cwd, bool captureOutput)
{
  // A relative path with a slash resolves against the child's cwd;
  // absolutize it up front so the POSIX pre-check (which runs in the parent)
  // agrees with the child's exec. Bare names (PATH search) are left as-is.
  std::string exe = path;
  if (!cwd.empty() && exe.find('/') != std::string::npos) {
    std::filesystem::path p{exe};
    if (!p.is_absolute())
      exe = (std::filesystem::path(cwd) / p).lexically_normal().string();
  }
#if defined(_WIN32)
  return spawnWin(exe, args, cwd, captureOutput);
#elif defined(ANDROID) || defined(__EMSCRIPTEN__)
  return spawnStub(exe, args, cwd, captureOutput);
#else
  return spawnPosix(exe, args, cwd, captureOutput);
#endif
}

bool terminate_process(int pid)
{
#if defined(_WIN32)
  return terminateWin(pid);
#elif defined(ANDROID) || defined(__EMSCRIPTEN__)
  return terminateStub(pid);
#else
  return terminatePosix(pid);
#endif
}

std::optional<int> try_reap(int pid, void* processHandle)
{
#if defined(_WIN32)
  return tryReapWin(pid, processHandle);
#elif defined(ANDROID) || defined(__EMSCRIPTEN__)
  return tryReapStub(pid, processHandle);
#else
  return tryReapPosix(pid, processHandle);
#endif
}

} // namespace launcher
} // namespace base
