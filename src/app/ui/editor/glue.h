// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/tools/pointer.h"
#include "app/ui/editor/editor.h"
#include "ui/message.h"

namespace app {

// Code to convert "ui" messages to "tools" mouse pointers

inline tools::Pointer::Button button_from_msg(ui::MouseMessage* msg) {
  return
    (msg->right() ? tools::Pointer::Right:
     (msg->middle() ? tools::Pointer::Middle:
                      tools::Pointer::Left));
}

inline tools::Pointer pointer_from_msg(Editor* editor, ui::MouseMessage* msg) {
  return
    tools::Pointer(editor->screenToEditor(msg->position()),
                   button_from_msg(msg),
                   msg->pointerType() == she::PointerType::Pen ? msg->pressure() : 1.0f);
}

} // namespace app
