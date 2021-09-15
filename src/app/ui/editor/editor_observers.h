// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/editor/editor_observer.h"
#include "base/observable.h"

namespace app {
  class Editor;

  class EditorObservers : public base::Observable<EditorObserver> {
  public:
    void notifyDestroyEditor(Editor* editor);
    void notifyStateChanged(Editor* editor);
    void notifyScrollChanged(Editor* editor);
    void notifyZoomChanged(Editor* editor);
    void notifyBeforeFrameChanged(Editor* editor);
    void notifyAfterFrameChanged(Editor* editor);
    void notifyBeforeLayerChanged(Editor* editor);
    void notifyAfterLayerChanged(Editor* editor);
  };

} // namespace app
