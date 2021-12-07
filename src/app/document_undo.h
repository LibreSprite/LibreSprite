// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "base/disable_copying.h"
#include "base/observable.h"
#include "doc/sprite_position.h"
#include "undo/undo_history.h"

#include <string>

namespace doc {
  class Context;
}

namespace app {
  using namespace doc;

  class Cmd;
  class CmdTransaction;
  class DocumentUndoObserver;

  class DocumentUndo : public base::Observable<DocumentUndoObserver> {
  public:
    DocumentUndo();

    void setContext(doc::Context* ctx);

    void add(CmdTransaction* cmd);

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    void clearRedo();

    bool isSavedState() const;
    void markSavedState();
    void impossibleToBackToSavedState();

    std::string nextUndoLabel() const;
    std::string nextRedoLabel() const;

    SpritePosition nextUndoSpritePosition() const;
    SpritePosition nextRedoSpritePosition() const;

    Cmd* lastExecutedCmd() const;

    int* savedCounter() { return &m_savedCounter; }

    const undo::UndoState* firstState() const { return m_undoHistory.firstState(); }
    const undo::UndoState* currentState() const { return m_undoHistory.currentState(); }

    void moveToState(const undo::UndoState* state);

  private:
    const undo::UndoState* nextUndo() const;
    const undo::UndoState* nextRedo() const;

    undo::UndoHistory m_undoHistory;
    doc::Context* m_ctx;

    // This counter is equal to 0 if we are in the "saved state", i.e.
    // the document on memory is equal to the document on disk. This
    // value is less than 0 if we're in a past version of the document
    // (due undoes), or greater than 0 if we are in a future version
    // (due redoes).
    int m_savedCounter;

    // True if the saved state was invalidated/corrupted/lost in some
    // way. E.g. If the save process fails.
    bool m_savedStateIsLost;

    DISABLE_COPYING(DocumentUndo);
  };

} // namespace app
