// Copyright (C) 2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "tests/test.h"

#include "app/file/extension_format.h"
#include "base/fs.h"
#include "base/path.h"

#include <archive.h>
#include <archive_entry.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

// Writes a ustar archive at `archivePath` containing one regular-file entry
// per {name, content} pair in `files`, plus an optional symlink entry.
void writeTestArchive(
    const std::string& archivePath,
    const std::vector<std::pair<std::string, std::string>>& files,
    const std::string& symlinkName = "",
    const std::string& symlinkTarget = "")
{
  archive* a = archive_write_new();
  archive_write_set_format_ustar(a);
  ASSERT_EQ(ARCHIVE_OK, archive_write_open_filename(a, archivePath.c_str()));

  for (auto& [name, content] : files)
  {
    archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, name.c_str());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, content.size());
    ASSERT_EQ(ARCHIVE_OK, archive_write_header(a, entry));
    archive_write_data(a, content.data(), content.size());
    archive_entry_free(entry);
  }

  if (!symlinkName.empty())
  {
    archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, symlinkName.c_str());
    archive_entry_set_filetype(entry, AE_IFLNK);
    archive_entry_set_symlink(entry, symlinkTarget.c_str());
    archive_entry_set_perm(entry, 0777);
    archive_entry_set_size(entry, 0);
    ASSERT_EQ(ARCHIVE_OK, archive_write_header(a, entry));
    archive_entry_free(entry);
  }

  archive_write_close(a);
  archive_write_free(a);
}

class ExtensionFormatTest : public ::testing::Test
{
protected:
  std::string tempDir;
  std::string archivePath;
  std::string destDir;

  void SetUp() override
  {
    tempDir = base::get_temp_path() + base::path_separator +
              "besprited_extension_format_test";
    std::filesystem::remove_all(tempDir);
    base::make_all_directories(tempDir);

    archivePath = tempDir + base::path_separator + "test.tar";
    destDir = tempDir + base::path_separator + "dest";
    base::make_all_directories(destDir);
  }

  void TearDown() override { std::filesystem::remove_all(tempDir); }
};

} // namespace

TEST_F(ExtensionFormatTest, ExtractsNestedRegularFilesAndCreatesDirectories)
{
  writeTestArchive(archivePath, {{"a.txt", "hello"},
                                 {"nested/dir/b.txt", "world"}});

  FILE* file = fopen(archivePath.c_str(), "rb");
  ASSERT_NE(nullptr, file);
  app::Archive archive{file};
  archive.extractTo(destDir);
  fclose(file);

  EXPECT_TRUE(base::is_file(destDir + base::path_separator + "a.txt"));
  EXPECT_TRUE(base::is_file(
      destDir + base::path_separator + "nested/dir/b.txt"));

  std::ifstream in(destDir + base::path_separator + "nested/dir/b.txt");
  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ("world", content);
}

TEST_F(ExtensionFormatTest, RejectsPathTraversalEntryAndWritesNothingOutside)
{
  // A crafted extension archive with a zip-slip entry escaping destDir.
  std::string outsideMarker = tempDir + base::path_separator + "escaped.txt";
  writeTestArchive(archivePath, {{"../escaped.txt", "pwned"}});

  FILE* file = fopen(archivePath.c_str(), "rb");
  ASSERT_NE(nullptr, file);
  app::Archive archive{file};
  EXPECT_THROW(archive.extractTo(destDir), std::runtime_error);
  fclose(file);

  EXPECT_FALSE(base::is_file(outsideMarker));
}

TEST_F(ExtensionFormatTest, RejectsAbsolutePathEntry)
{
  writeTestArchive(archivePath, {{"/etc/passwd_clobber", "pwned"}});

  FILE* file = fopen(archivePath.c_str(), "rb");
  ASSERT_NE(nullptr, file);
  app::Archive archive{file};
  EXPECT_THROW(archive.extractTo(destDir), std::runtime_error);
  fclose(file);
}

TEST_F(ExtensionFormatTest, SkipsSymlinkEntriesInsteadOfExtractingThem)
{
  writeTestArchive(archivePath, {{"safe.txt", "ok"}}, "link.txt",
                   "/etc/passwd");

  FILE* file = fopen(archivePath.c_str(), "rb");
  ASSERT_NE(nullptr, file);
  app::Archive archive{file};
  archive.extractTo(destDir);
  fclose(file);

  EXPECT_TRUE(base::is_file(destDir + base::path_separator + "safe.txt"));
  EXPECT_FALSE(
      std::filesystem::exists(destDir + base::path_separator + "link.txt"));
}
