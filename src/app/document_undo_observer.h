// Aseprite
// Copyright (C) 2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace undo {
  class UndoState;
}

namespace app {

class DocumentUndo;

  class DocumentUndoObserver {
  public:
    virtual ~DocumentUndoObserver() { }
    virtual void onAddUndoState(DocumentUndo* history) = 0;
    virtual void onAfterUndo(DocumentUndo* history) = 0;
    virtual void onAfterRedo(DocumentUndo* history) = 0;
    virtual void onClearRedo(DocumentUndo* history) = 0;
  };

} // namespace app
