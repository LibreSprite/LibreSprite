// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2018-2022  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/resource_finder.h"

#include "app/app.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/string.h"

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#endif

namespace app {

ResourceFinder::ResourceFinder(bool log)
  : m_log(log)
{
  m_current = -1;
}

const std::string& ResourceFinder::filename() const
{
  // Throw an exception if we are out of bounds
  return m_paths.at(m_current);
}

const std::string& ResourceFinder::defaultFilename() const
{
  if (m_default.empty()) {
    // The first path is the default one if nobody specified it.
    if (!m_paths.empty())
      return m_paths[0];
  }
  return m_default;
}

bool ResourceFinder::next()
{
  ++m_current;
  return (m_current < (int)m_paths.size());
}

bool ResourceFinder::findFirst()
{
  while (next()) {
    if (m_log)
      LOG("Searching file \"%s\"...", filename().c_str());

    if (base::is_file(filename())) {
      if (m_log)
        LOG(" (found)\n");

      return true;
    }
    else if (m_log)
      LOG(" (not found)\n");
  }

  return false;
}

void ResourceFinder::addPath(const std::string& path)
{
  m_paths.push_back(path);
}

void ResourceFinder::includeBinDir(const char* filename)
{
  addPath(base::join_path(base::get_file_path(base::get_app_path()), filename));
}

void ResourceFinder::includeDataDir(const char* filename)
{
  char buf[4096];

#ifdef _WIN32

  sprintf(buf, "data/%s", filename);
  includeHomeDir(buf); // %AppData%/LibreSprite/data/filename
  includeBinDir(buf);  // $BINDIR/data/filename

#elif __APPLE__

  sprintf(buf, "data/%s", filename);
  includeUserDir(buf); // $HOME/Library/Application Support/LibreSprite/data/filename
  includeBinDir(buf);  // $BINDIR/data/filename (outside the bundle)

  sprintf(buf, "../Resources/data/%s", filename);
  includeBinDir(buf);  // $BINDIR/../Resources/data/filename (inside a bundle)

  // $BINDIR/../share/libresprite/data/filename (installed in /usr/ or /usr/local/)
  sprintf(buf, "../share/libresprite/data/%s", filename);
  includeBinDir(buf);
  
#else


  const char* xdgdir = std::getenv("XDG_CONFIG_HOME");
  if((xdgdir) && (*xdgdir)) {
    sprintf(buf, "%s/libresprite/data/%s", xdgdir, filename); // $XDG_CONFIG_HOME/libresprite/data/filename
    addPath(buf);
  }
  else {
    sprintf(buf, ".config/libresprite/data/%s", filename); // $HOME/.config/libresprite/data/filename
    includeHomeDir(buf);
  }

  // $BINDIR/data/filename
  sprintf(buf, "data/%s", filename);
  includeBinDir(buf);

  // $BINDIR/../share/libresprite/data/filename (installed in /usr/ or /usr/local/)
  sprintf(buf, "../share/libresprite/data/%s", filename);
  includeBinDir(buf);

#endif
}

void ResourceFinder::includeHomeDir(const char* filename)
{
#ifdef _WIN32

  // %AppData%/LibreSprite/filename
  wchar_t* env = _wgetenv(L"AppData");
  if (env) {
    std::string path = base::join_path(base::to_utf8(env), "LibreSprite");
    path = base::join_path(path, filename);
    addPath(path);
    m_default = path;
  }

#else

  char* env = std::getenv("HOME");
  char buf[4096];

  if ((env) && (*env)) {
    // $HOME/filename
    sprintf(buf, "%s/%s", env, filename);
    addPath(buf);
  }
  else {
    LOG("You don't have set $HOME variable\n");
    addPath(filename);
  }

#endif
}

void ResourceFinder::includeUserDir(const char* filename)
{
#ifdef _WIN32

  if (App::instance()->isPortable()) {
    // $BINDIR/filename
    includeBinDir(filename);
  }
  else {
    // %AppData%/LibreSprite/filename
    includeHomeDir(filename);
  }

#elif __APPLE__

  // $HOME/Library/Application Support/LibreSprite/filename
  addPath(
    base::join_path(
      base::join_path(base::get_lib_app_support_path(), PACKAGE),
      filename).c_str());

#else

  char buf[4096];
  const char* xdgdir = std::getenv("XDG_CONFIG_HOME");
  if((xdgdir) && (*xdgdir)) {
    sprintf(buf, "%s/libresprite/%s", xdgdir, filename); // $XDG_CONFIG_HOME/libresprite/filename
    addPath(buf);
  }
  else
    includeHomeDir((std::string(".config/libresprite/") + filename).c_str());   // $HOME/.config/libresprite/filename


#endif
}

void ResourceFinder::includeDesktopDir(const char* filename)
{
#ifdef _WIN32

  std::vector<wchar_t> buf(MAX_PATH);
  HRESULT hr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                               SHGFP_TYPE_DEFAULT, &buf[0]);
  if (hr == S_OK) {
    addPath(base::join_path(base::to_utf8(&buf[0]), filename));
  }
  else {
    includeHomeDir(filename);
  }

#elif defined(__APPLE__)

  // TODO get the desktop folder
  // $HOME/Desktop/filename
  includeHomeDir(base::join_path(std::string("Desktop"), filename).c_str());

#else

  char* desktopDir = std::getenv("XDG_DESKTOP_DIR");
  if (desktopDir) {
    // $XDG_DESKTOP_DIR/filename
    addPath(base::join_path(desktopDir, filename));
  }
  else {
    // $HOME/Desktop/filename
    includeHomeDir(base::join_path(std::string("Desktop"), filename).c_str());
  }

#endif
}

std::string ResourceFinder::getFirstOrCreateDefault()
{
  std::string fn;

  // Search from first to last path
  if (findFirst())
    fn = filename();

  // If the file wasn't found, we will create the directories for the
  // default file name.
  if (fn.empty()) {
    fn = defaultFilename();

    std::string dir = base::get_file_path(fn);
    if (!base::is_directory(dir))
      base::make_all_directories(dir);
  }

  return fn;
}

} // namespace app
