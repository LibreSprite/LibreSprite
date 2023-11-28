// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <climits>              // Required for PATH_MAX
#include <cstdio>               // Required for rename()
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <vector>

#if __APPLE__
#include <mach-o/dyld.h>
#elif __FreeBSD__
#include <sys/sysctl.h>
#endif

#include "base/path.h"
#include "base/time.h"

#define MAXPATHLEN 1024

#if defined(ANDROID)
extern std::string _AndroidDataDir;
extern std::string _AndroidStorageDir;
#endif

namespace base {

bool is_file(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISREG(sts.st_mode)) ? true: false;
}

bool is_directory(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISDIR(sts.st_mode)) ? true: false;
}

void make_directory(const std::string& path)
{
  int result = mkdir(path.c_str(), 0777);
  if (result < 0) {
    // TODO add errno into the exception
    throw std::runtime_error("Error creating directory");
  }
}

size_t file_size(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0) ? sts.st_size: 0;
}

void move_file(const std::string& src, const std::string& dst)
{
  int result = std::rename(src.c_str(), dst.c_str());
  if (result != 0)
    // TODO add errno into the exception
    throw std::runtime_error("Error moving file");
}

void delete_file(const std::string& path)
{
  int result = unlink(path.c_str());
  if (result != 0)
    // TODO add errno into the exception
    throw std::runtime_error("Error deleting file");
}

bool has_readonly_attr(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && ((sts.st_mode & S_IWUSR) == 0));
}

void remove_readonly_attr(const std::string& path)
{
  struct stat sts;
  int result = stat(path.c_str(), &sts);
  if (result == 0) {
    result = chmod(path.c_str(), sts.st_mode | S_IWUSR);
    if (result != 0)
      // TODO add errno into the exception
      throw std::runtime_error("Error removing read-only attribute");
  }
}

Time get_modification_time(const std::string& path)
{
  struct stat sts;
  int result = stat(path.c_str(), &sts);
  if (result != 0)
    return Time();

  std::tm* t = std::localtime(&sts.st_mtime);
  return Time(
    t->tm_year+1900, t->tm_mon+1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);
}

void remove_directory(const std::string& path)
{
  int result = rmdir(path.c_str());
  if (result != 0) {
    // TODO add errno into the exception
    throw std::runtime_error("Error removing directory");
  }
}

std::string get_current_path()
{
  std::vector<char> path(MAXPATHLEN);
  getcwd(&path[0], path.size());
  return std::string(&path[0]);
}

std::string get_app_path()
{
  std::vector<char> path(MAXPATHLEN);

#if __APPLE__
  uint32_t size = path.size();
  while (_NSGetExecutablePath(&path[0], &size) == -1)
    path.resize(size);
#elif __FreeBSD__
  size_t size = path.size();
  const int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
  while (sysctl(mib, 4, &path[0], &size, NULL, 0) == -1)
      path.resize(size);
#elif defined(ANDROID)
  return _AndroidDataDir + "/";
#else  /* linux */
  readlink("/proc/self/exe", &path[0], path.size());
#endif

  return std::string(&path[0]);
}

std::string get_temp_path()
{
  char* tmpdir = getenv("TMPDIR");
  if (tmpdir)
    return tmpdir;
  else
    return "/tmp";
}

std::string get_user_docs_folder()
{
#ifdef ANDROID
  return _AndroidStorageDir;
#else
  char* tmpdir = getenv("HOME");
  if (tmpdir)
    return tmpdir;
  else
    return "/";
#endif
}

std::string get_canonical_path(const std::string& path)
{
  char buffer[PATH_MAX];
  if (realpath(path.c_str(), buffer) == NULL) {
    return path;
  }
  return buffer;
}

std::vector<std::string> list_files(const std::string& path)
{
  std::vector<std::string> files;
  DIR* handle = opendir(path.c_str());
  if (handle) {
    dirent* item;
    while ((item = readdir(handle)) != nullptr) {
      std::string filename = item->d_name;
      if (filename != "." && filename != "..")
        files.push_back(filename);
    }

    closedir(handle);
  }
  return files;
}

#if !__APPLE__

std::vector<std::string> get_font_paths()
{
    return {
        "/usr/share/fonts",
        "/usr/local/share/fonts",
        "~/.local/share/fonts",
        "~/.fonts",
        "/usr/share/fonts/OTF",
        "/usr/local/share/fonts/OTF",
        "~/.local/share/fonts/OTF",
        "/usr/share/fonts/OTF",
        "/usr/local/share/fonts/OTF",
        "~/.local/share/fonts/OTF",
        "~/.font/OTF",
        "/usr/share/fonts/TTF",
        "/usr/local/share/fonts/TTF",
        "~/.local/share/fonts/TTF",
        "~/.font/TTF",
        "/usr/share/fonts/ttf",
        "/usr/local/share/fonts/ttf",
        "~/.local/share/fonts/ttf",
        "~/.font/ttf"
    };
}
#endif

}
