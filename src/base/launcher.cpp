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

#include <cstdlib>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <spawn.h>
#include <sys/wait.h>

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
  if (base::is_directory(file))
  {
    ret = spawn_and_wait({"open", file});
  }
  else
  {
    ret = spawn_and_wait({"open", "--reveal", file});
  }
  return (ret == 0);

#else

  if (!base::is_directory(file))
    file = base::get_file_path(file);

  int ret = spawn_and_wait({"xdg-open", file});
  return (ret == 0);

#endif
}

} // namespace launcher
} // namespace base
