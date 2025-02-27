// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C) 2018-2022  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <stdexcept>
#include <windows.h>
#include <shlobj.h>
#include <sys/stat.h>
#include <filesystem>

#include "base/path.h"
#include "base/string.h"
#include "base/win32_exception.h"
#include "base/time.h"

namespace base {

bool is_file(const std::string& path)
{
  DWORD attr = ::GetFileAttributesW(from_utf8(path).c_str());

  // GetFileAttributes returns INVALID_FILE_ATTRIBUTES in case of
  // fail.
  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool is_directory(const std::string& path)
{
  DWORD attr = ::GetFileAttributesW(from_utf8(path).c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
}

size_t file_size(const std::string& path)
{
  struct _stat sts;
  return (_wstat(from_utf8(path).c_str(), &sts) == 0) ? sts.st_size: 0;
}

void move_file(const std::string& src, const std::string& dst)
{
  BOOL result = ::MoveFileW(from_utf8(src).c_str(), from_utf8(dst).c_str());
  if (result == 0)
    throw Win32Exception("Error moving file");
}

void copy_file(const std::string& src, const std::string& dst)
{
    std::filesystem::copy_file(src, dst);
}

void delete_file(const std::string& path)
{
  BOOL result = ::DeleteFileW(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error deleting file");
}

bool has_readonly_attr(const std::string& path)
{
  std::wstring fn = from_utf8(path);
  DWORD attr = ::GetFileAttributesW(fn.c_str());
  return ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY);
}

void remove_readonly_attr(const std::string& path)
{
  std::wstring fn = from_utf8(path);
  DWORD attr = ::GetFileAttributesW(fn.c_str());
  if ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
    ::SetFileAttributesW(fn.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
}

Time get_modification_time(const std::string& path)
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  ZeroMemory(&data, sizeof(data));

  std::wstring fn = from_utf8(path);
  if (!GetFileAttributesExW(fn.c_str(), GetFileExInfoStandard, (LPVOID)&data))
    return Time();

  SYSTEMTIME utc, local;
  FileTimeToSystemTime(&data.ftLastWriteTime, &utc);
  SystemTimeToTzSpecificLocalTime(NULL, &utc, &local);

  return Time(
    local.wYear, local.wMonth, local.wDay,
    local.wHour, local.wMinute, local.wSecond);
}

void make_directory(const std::string& path)
{
  BOOL result = ::CreateDirectoryW(from_utf8(path).c_str(), NULL);
  if (result == 0)
    throw Win32Exception("Error creating directory");
}

void remove_directory(const std::string& path)
{
  BOOL result = ::RemoveDirectoryW(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error removing directory");
}

std::string get_current_path()
{
  return std::filesystem::current_path().string();
}

std::string get_app_path()
{
  WCHAR buffer[MAX_PATH+1];
  if (::GetModuleFileNameW(NULL, buffer, sizeof(buffer)/sizeof(WCHAR)))
    return to_utf8(buffer);
  else
    return "";
}

std::string get_temp_path()
{
  WCHAR buffer[MAX_PATH+1];
  DWORD result = ::GetTempPathW(sizeof(buffer)/sizeof(WCHAR), buffer);
  return to_utf8(buffer);
}

std::string get_user_docs_folder()
{
  WCHAR buffer[MAX_PATH+1];
  HRESULT hr = SHGetFolderPathW(
    NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT,
    buffer);
  if (hr == S_OK)
    return to_utf8(buffer);
  else
    return "";
}

std::string get_canonical_path(const std::string& path)
{
  WCHAR buffer[MAX_PATH+1];
  GetFullPathNameW(
    from_utf8(path).c_str(),
    sizeof(buffer)/sizeof(WCHAR),
    buffer,
    nullptr);
  return to_utf8(buffer);
}

std::vector<std::string> list_files(const std::string& path)
{
  WIN32_FIND_DATAW fd;
  std::vector<std::string> files;
  HANDLE handle = FindFirstFileW(base::from_utf8(base::join_path(path, "*")).c_str(), &fd);
  if (handle) {
    do {
      std::string filename = base::to_utf8(fd.cFileName);
      if (filename != "." && filename != "..")
        files.push_back(filename);
    } while (FindNextFileW(handle, &fd));
    FindClose(handle);
  }
  return files;
}

std::vector<std::string> get_font_paths()
{
    std::vector<wchar_t> buf(MAX_PATH);
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL,
                                 SHGFP_TYPE_DEFAULT, &buf[0]);
    if (hr == S_OK) {
        return {base::to_utf8(&buf[0])};
    }
    return {};
}

}
