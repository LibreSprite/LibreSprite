// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/mutex.h"

#include <string>
#include <vector>

namespace she {
  class Surface;
}

namespace app {

  class IFileItem;

  typedef std::vector<IFileItem*> FileItemList;

  class FileSystemModule {
    static FileSystemModule* m_instance;

  public:
    FileSystemModule();
    ~FileSystemModule();

    static FileSystemModule* instance();

    // Marks all FileItems as deprecated to be refresh the next time
    // they are queried through @ref FileItem::children().
    void refresh();

    IFileItem* getRootFileItem();

    // Returns the FileItem through the specified "path".
    // Warning: You have to call path.fix_separators() before.
    IFileItem* getFileItemFromPath(const std::string& path);

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

  private:
    base::mutex m_mutex;
  };

  class LockFS {
  public:
    LockFS(FileSystemModule* fs) : m_fs(fs) {
      m_fs->lock();
    }
    ~LockFS() {
      m_fs->unlock();
    }
  private:
    FileSystemModule* m_fs;
  };

  class IFileItem {
  public:
    virtual ~IFileItem() { }

    virtual bool isFolder() const = 0;
    virtual bool isBrowsable() const = 0;
    virtual bool isHidden() const = 0;

    virtual std::string keyName() const = 0;
    virtual std::string fileName() const = 0;
    virtual std::string displayName() const = 0;

    virtual IFileItem* parent() const = 0;
    virtual const FileItemList& children() = 0;
    virtual void createDirectory(const std::string& dirname) = 0;

    virtual bool hasExtension(const std::string& csv_extensions) = 0;

    virtual she::Surface* getThumbnail() = 0;
    virtual void setThumbnail(she::Surface* thumbnail) = 0;
  };

} // namespace app
