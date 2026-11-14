// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace base {
namespace launcher {

bool open_url(const std::string& url);
bool open_file(const std::string& file);
bool open_folder(const std::string& file);

// A spawned process (see spawn_process).
struct ProcessInfo {
  int pid = -1;                  // -1 if the spawn failed
  void* processHandle = nullptr; // Windows: process handle, kept for reaping
  void* outputPipe = nullptr;    // Windows: read end of the capture pipe
  int outputPipeFd = -1;         // POSIX: read end of the capture pipe
  std::string error;             // why the spawn failed (empty on success)
};

// Spawns a detached child process: no wait, no shell (argv is passed as-is,
// same rationale as spawn_and_wait). The child inherits the environment and,
// unless captureOutput, the app's stdio. cwd is the child's working
// directory ("") means inherit; a relative path with a slash is resolved
// against cwd, a bare name is searched in PATH. On failure, pid is -1 and
// error is set.
ProcessInfo spawn_process(const std::string& path,
                          const std::vector<std::string>& args,
                          const std::string& cwd,
                          bool captureOutput);

// Terminates a process (SIGTERM on POSIX, TerminateProcess on Windows).
// False if the process does not exist.
bool terminate_process(int pid);

// Non-blocking reap of a spawned child: waitpid(pid, WNOHANG) on POSIX,
// WaitForSingleObject on Windows. Returns the exit code if the process has
// exited (a terminating signal becomes 128+signal; on Windows the handle
// is closed), std::nullopt if it is still running.
std::optional<int> try_reap(int pid, void* processHandle);

} // namespace launcher
} // namespace base
