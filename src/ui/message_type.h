// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace ui {

  // Message types.
  enum MessageType {
    // General messages.
    kOpenMessage,     // Windows is open.
    kCloseMessage,    // Windows is closed.
    kCloseDisplayMessage, // The user wants to close the entire application.
    kResizeDisplayMessage,
    kPaintMessage,    // Widget needs be repainted.
    kTimerMessage,    // A timer timeout.
    kDropFilesMessage, // Drop files in the manager.
    kWinMoveMessage,  // Window movement.

    // Keyboard related messages.
    kKeyDownMessage,         // When a any key is pressed.
    kKeyUpMessage,           // When a any key is released.
    kFocusEnterMessage,      // Widget gets the focus.
    kFocusLeaveMessage,      // Widget losts the focus.

    // Mouse related messages.
    kMouseDownMessage,      // User makes click inside a widget.
    kMouseUpMessage,        // User releases mouse button in a widget.
    kDoubleClickMessage,    // User makes double click in some widget.
    kMouseEnterMessage,     // A widget gets mouse pointer.
    kMouseLeaveMessage,     // A widget losts mouse pointer.
    kMouseMoveMessage,      // User moves the mouse on some widget.
    kSetCursorMessage,      // A widget needs to setup the mouse cursor.
    kMouseWheelMessage,     // User moves the wheel.

    // Touch related messages.
    kTouchMagnifyMessage,

    // TODO Drag'n'drop messages...
    // k...DndMessage

    // User widgets.
    kFirstRegisteredMessage,
    kLastRegisteredMessage = 0x7fffffff
  };

} // namespace ui
