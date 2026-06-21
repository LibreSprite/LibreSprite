// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/file_selector.h"
#include "ui/window.h"

#include "file_selector.xml.h"

#include <string>

namespace ui {
  class Button;
  class ComboBox;
  class Entry;
}

namespace app {
  class CustomFileNameEntry;
  class FileList;
  class FileListView;
  class IFileItem;

  class FileSelector : public app::gen::FileSelector {
  public:
    FileSelector(FileSelectorType type, FileSelectorDelegate* delegate);

    void goBack();
    void goForward();
    void goUp();
    void goInsideFolder();

    // Shows the dialog to select a file in the program.
    std::string show(const std::string& title,
                     const std::string& initialPath,
                     const std::string& showExtensions);

    FileList* fileList() {return m_fileList;}

  private:
    void updateLocation();
    void updateNavigationButtons();
    void addInNavigationHistory(IFileItem* folder);
    void onShowHiddenFiles();
    void onGoBack();
    void onGoForward();
    void onGoUp();
    void onNewFolder();
    void onLocationCloseListBox();
    void onFileTypeChange();
    void onFileListFileSelected();
    void onFileListFileAccepted();
    void onFileListCurrentFolderChanged();
    std::string getSelectedExtension() const;
    std::string WRITE_YOUR_OWN_PATH_NAME = "WRITE YOUR OWN PATH";

    FileSelectorType m_type;
    FileSelectorDelegate* m_delegate;
    std::string m_defExtension;
    CustomFileNameEntry* m_fileName;
    FileList* m_fileList;
    FileListView* m_fileView;

    // If true the navigation_history isn't
    // modified if the current folder changes
    // (used when the back/forward buttons
    // are pushed)
    bool m_navigationLocked;
  };

} // namespace app
